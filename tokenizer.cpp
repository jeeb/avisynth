// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

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


#include "tokenizer.h"



/****************************
 *******   Tokenizer   ******
 ***************************/

Tokenizer::Tokenizer(const char* pc, IScriptEnvironment* _env) 
  : env(_env) 
{
  this->pc = pc;
  this->line = 1;
  this->type = 0;
  NextToken();
}

Tokenizer::Tokenizer(Tokenizer* old) 
  : env(old->env) 
{
  pc = old->pc;
  line = old->line;
  NextToken();
}  


void Tokenizer::NextToken() {

  if (IsNewline())
    line++;

  // skip whitespace, comments, and escaped newlines

  for (;;) {
    SkipWhitespace();
    if (*pc == '\\') {
      // eat backslash followed by newline
      const char* const old_pc = pc;
      pc++;
      SkipWhitespace();
      if (*pc == '\n' || *pc == '\r') {
        SkipNewline();
        continue;
      } else {
        token_start = old_pc;
        env->ThrowError("Script error: `\\' can only appear at the beginning or end of a line");
      }
    } else if (*pc == '\n' || *pc == '\r') {
      // skip newline if it's followed by backslash or `{'
      const char* const old_pc = pc;
      const int old_line = line;
      do {
        SkipNewline();
        SkipWhitespace();
      } while (*pc == '\n' || *pc == '\r');
      if (*pc == '\\') {
        pc++;
        continue;
      } else if (*pc == '{') {
        break;
      } else {
        pc = old_pc;
        line = old_line;
        break;
      }
    } else if (*pc == '#') {
      // skip from # to end of line (comment)
      while (*pc != 0 && *pc != '\n' && *pc != '\r')
        pc++;
      break;
    } else {
      break;
    }
  }

  token_start = pc;

  switch (*pc) {

    case 0:
      type = 0;
      break;

    case '\n': case '\r':
      SkipNewline();
      type = 'n';
      line--;
      break;

    case '.':
      // a '.' followed by a digit is a number, otherwise it's an operator
      if (isdigit(pc[1])) {
        GetNumber();
      } else {
        ++pc;
        SetToOperator('.');
      }
      break;

    case '<':    // these operators have versions followed by '='
    case '>':
    case '!':
    case '=':
      if (pc[1] != '=') {
        SetToOperator(*pc++);
      } else {
        SetToOperator(pc[0] * 256 + pc[1]);
        pc += 2;
      }
      break;

    case '+':    // these operators have single and double (++, &&, ||, ==) versions
    case '&':
    case '|':
      if (pc[1] != pc[0]) {
        SetToOperator(*pc++);
      } else {
        SetToOperator(pc[0] * 256 + pc[1]);
        pc += 2;
      }
      break;

    case '{':    // these operators are always lone characters
    case '}':
    case '(':
    case ')':
    case ',':
    case '?':
    case ':':
    case '-':
    case '*':
    case '/':
    case '%':
      SetToOperator(*pc++);
      break;

    case '$':    // hexadecimal number
      type = 'i';
      integer = 0;
      ++pc;
      do {
        if (*pc >= '0' && *pc <= '9')
          integer = integer*16 + (*pc - '0');
        else if (*pc >= 'a' && *pc <= 'f')
          integer = integer*16 + (*pc - 'a' + 10);
        else if (*pc >= 'A' && *pc <= 'F')
          integer = integer*16 + (*pc - 'A' + 10);
        else
          env->ThrowError("$ must be followed by a hexadecimal number");
      } while (isalnum(*++pc));
      break;

    case '"':    // string
      {
        const char *start, *end;
        if (pc[1] == '"' && pc[2] == '"') {
          // """..."""
          start = pc+3;
          end = strstr(start, "\"\"\"");
          if (!end)
            env->ThrowError("Parse error: string missing closing quotation marks");
          while (end[3] == '"')
            end++;
          pc = end+3;
        } else {
          // "..."
          start = pc+1;
          end = strchr(start, '"');
          if (!end)
            env->ThrowError("Parse error: string missing closing quotation mark");          
          
          /* I like the ability to have newlines in strings, thanks */
          // const char *cr = strchr(start, '\r'), *lf = strchr(start, '\n');          
          // if ((cr && cr < end) || (lf && lf < end))
          //   env->ThrowError("Parse error: newline found in string");
          
          pc = end+1;
        }
        type = 's';
        string = env->SaveString(start, end-start);
      }
      break;

    default:
      if (isdigit(*pc)) {
        // number
        GetNumber();
      } else if (*pc == '_' || isalpha(*pc)) {
        // identifier
        do {
          pc++;
        } while (*pc == '_' || isalnum(*pc));
        type = 'd';
        identifier = env->SaveString(token_start, pc - token_start);
        if (!lstrcmpi(identifier, "__END__")) {
          type = 0;
        }
      } else {
        env->ThrowError("unexpected character \"%c\"", *pc);
      }
      break;
  }
}


int Tokenizer::GetColumn(const char* start_of_string) const
{
    const char* x = pc;
    while (x > start_of_string && x[-1] != '\n' && x[-1] != '\r')
      x--;
    return pc-x;
}


void Tokenizer::SkipWhitespace() 
{
  while (*pc == ' ' || *pc == '\t')
    pc++;
}

void Tokenizer::SkipNewline() 
{
  if (*pc == '\n' || *pc == '\r') 
  {
    pc++;
    line++;
    if ((*pc == '\n' || *pc == '\r') && *pc != *(pc-1))
      pc++;
  }
}

void Tokenizer::AssertType(char expected_type) const 
{
  if (type != expected_type)
    ThrowTypeMismatch(expected_type, type, env);
}

void Tokenizer::GetNumber() 
{
  // start by assuming an int and switch to float if necessary
  type = 'i';
  integer=0;
  do {
    if (*pc == '.') {
      type = 'f';
      floating_pt = float(integer);
      float place = 1;
      ++pc;
      while (isdigit(*pc)) {
        place /= 10;
        floating_pt += place * (*pc - '0');
        ++pc;
      }
      break;
    } else {
      integer = integer*10 + (*pc - '0');
    }
    ++pc;
  } while (isdigit(*pc) || *pc == '.');
}


void Tokenizer::SetToOperator(int o) 
{
  type = 'o';
  op = o;
}



/**** Helper Functions ****/

const char* GetTypeName(short type) 
{
  switch (type) 
  {
    case 0:   return "undefined";
    case 'a': return "array";
    case 'b': return "boolean";
    case 'c': return "clip";
    case 'd': return "identifier";
    case 'f': return "floating-point";
    case 'i': return "integer";
    case 'o': return "operator";
    case 's': return "string";
    default: return "unknown";
  }
}


void ThrowTypeMismatch(char expected, char actual, IScriptEnvironment* env) 
{
  env->ThrowError("Tokenizer: expected type '%s' doesn't match actual type '%s' (this is a bug)",
      GetTypeName(expected), GetTypeName(actual));
}

