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

#ifndef __ScriptParser_H__
#define __ScriptParser_H__

#include "internal.h"
#include "expression.h"
#include "tokenizer.h"
#include "script.h"


/********************************************************************
********************************************************************/



class ScriptParser 
/**
  * Insert intelligent comment here
 **/
{
public:
  ScriptParser(IScriptEnvironment* _env, const char* _code, const char* _filename);

  PExpression Parse(void);

private:
  IScriptEnvironment* const env;
  Tokenizer tokenizer;
  const char* const code;
  const char* const filename;

  enum {max_args=60};

  void Expect(int op, const char* msg);

  void ParseFunctionDefinition(void);
  
  PExpression ParseBlock(bool braced);
  PExpression ParseStatement(bool* stop);
  PExpression ParseAssignment(void);
  PExpression ParseConditional(void);
  PExpression ParseOr(void);
  PExpression ParseAnd(void);
  PExpression ParseComparison(void);
  PExpression ParseAddition(void);
  PExpression ParseMultiplication(void);
  PExpression ParseUnary(void);
  PExpression ParseOOP(void);

  PExpression ParseFunction(PExpression context);
  PExpression ParseAtom(void);

  // helper for ParseComparison
  int GetTokenAsComparisonOperator();
};




#endif  // __ScriptParser_H__