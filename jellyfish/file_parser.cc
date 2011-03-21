/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <jellyfish/file_parser.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

namespace jellyfish {
  file_parser *file_parser::new_file_parser_sequence(const char *path) {
    int fd = open(path, O_RDONLY);
    if(fd == -1)
      throw_perror<FileParserError>("Error opening file '%s'", path);
      
    char peek;
    if(read(fd, &peek, 1) <= 0)
      throw_error<FileParserError>("Empty input file '%s'", path);

    switch(peek) {
    case '>': return new fasta_parser(fd, path, &peek, 1);
    case '@': return new fastq_sequence_parser(fd, path, &peek, 1);
      
    default:
      throw_error<FileParserError>("Invalid input file '%'", path);
    }
    // Should never be reached
    return 0;
  }

  file_parser *file_parser::new_file_parser_seq_qual(const char *path) {
    int fd = open(path, O_RDONLY);
    if(fd == -1)
      throw_perror<FileParserError>("Error opening file '%s'", path);
      
    char peek;
    if(read(fd, &peek, 1) <= 0)
      throw_error<FileParserError>("Empty input file '%s'", path);

    switch(peek) {
    case '@': return new fastq_seq_qual_parser(fd, path, &peek, 1);
      
    default:
      throw_error<FileParserError>("Invalid input file '%'", path);
    }
    // Should never be reached
    return 0;
  }


  file_parser::file_parser(int fd, const char *path, 
                           const char *str, size_t len, char pbase) : 
    _fd(fd), _base(pbase), _pbase(pbase) {
    struct stat stat_buf;
    if(fstat(fd, &stat_buf) == -1)
      throw_perror<FileParserError>("Can't fstat '%s'", path);
    _size       = stat_buf.st_size;
    _buffer     = (char *)mmap(0, _size , PROT_READ, MAP_SHARED, fd, 0);
    _is_mmapped = _buffer != MAP_FAILED;
    if(_is_mmapped) {
      _end_buffer = _buffer + _size;
      _data       = _buffer;
      _end_data   = _end_buffer;
      close(_fd);
    } else {
      _buffer     = new char[_buff_size];
      _end_buffer = _buffer + _buff_size;
      _data       = _end_data = _buffer;
      // What if len > _buff_size!!!
      if(str && len) {
        memcpy(_buffer, str, len);
        _end_data   = _buffer + len;
      }        
    }
  }

  file_parser::~file_parser() {
    if(_is_mmapped) {
      munmap(_buffer, _size);
    } else {
      delete _buffer;
      close(_fd);
    }
  }

  // Get next character in "stream"
  int file_parser::sbumpc() {
    _pbase = _base;
    if(_data >= _end_data) {
      if(_is_mmapped)
        return (_base = EOF);
      ssize_t gcount = read(_fd, _buffer, _buff_size);
      if(gcount <= 0)
        return (_base = EOF);
      _data     = _buffer;
      _end_data = _buffer + gcount;
    }
    //    std::cerr << "sbumpc " << *_data << std::endl;
    return (_base = *_data++);
  }
}