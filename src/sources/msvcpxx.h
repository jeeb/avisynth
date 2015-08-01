// Avisynth v2.6.  Copyright 2015 Ian Brabham.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making
// a combined work based on Avisynth.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of
// the license terms of these independent modules, and to copy and distribute
// the resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

#ifndef __msvcpXX_H__
#define __msvcpXX_H__


#include <fcntl.h>


// Quick replacement for msvcp*.dll as used in ImageSeq.{cpp,h}


class ios_base {
public:
  enum {
    cur = 1
  };

};

class ios {
public:
  enum {
    beg = 2,
    binary = 4,
    out = 8,
    trunc = 16
  };

};


class stream {
  friend class istream;
  friend class ostream;
  friend class ifstream;
  friend class ofstream;

  stream() : file(-1) { };

  int file;

public:
  ~stream() {
    close();
  }

  void close() {
    if (file != -1) {
      _close(file);
      file = -1;
    }
  };

  bool is_open() {
    return (file != -1);
  };

  void seekg(long offset, int mode) {
    switch (mode) {
      case ios_base::cur : _lseek(file, offset, SEEK_CUR); break;
      case ios::beg      : _lseek(file, offset, SEEK_SET); break;
      default            :                                 break;
    }
  };

  bool operator ! () {
    return (file != -1);
  };

};

class ostream : public stream {
public:
  void write(const void* buffer, size_t count) {
    _write(file, buffer, count);
  };

};

class istream : public stream {
public:
  void read(void* buffer, size_t count) {;
    _read(file, buffer, count);
  }

};

class ofstream : public ostream {
public:
  ofstream(const char* filename, int mode) {
    file = _open(filename, _O_WRONLY|_O_BINARY|_O_SEQUENTIAL|_O_CREAT|_O_TRUNC);
  };

};

class ifstream : public istream {
public:
  ifstream(const char* filename, int mode) {;
    file = _open(filename, _O_RDONLY|_O_BINARY|_O_SEQUENTIAL);
  };

};


class ostringstream {
private:
  enum {size = 511 };
  char buffer[size+1];

public:
  ostringstream() { buffer[0] = 0; };

  ostringstream& str() { return *this; };

  char* c_str() {
    buffer[size] = 0;
    return buffer;
  };

  ostringstream& operator << (const char* text) {
    strcat(buffer, text);
    return *this;
  };

  ostringstream& operator << (const int number) {
    _itoa(number, buffer+strlen(buffer), 10);
    return *this;
  };

};

#endif // __msvcpXX_H__
