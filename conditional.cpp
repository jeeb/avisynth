
// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
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
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

#include "stdafx.h"

#include "conditional.h"
#include "scriptparser.h"
#include "text-overlay.h"


AVSFunction Conditional_filters[] = {
  {  "ConditionalFilter","cccsss[show]b", ConditionalFilter::Create },
  { 0 }
};


ConditionalFilter::ConditionalFilter(PClip _child, PClip _source1, PClip _source2, AVSValue  _condition1, AVSValue  _evaluator, AVSValue  _condition2, bool _show, IScriptEnvironment* env) :
	GenericVideoFilter(_child), source1(_source1), source2(_source2),
	eval1(_condition1), eval2(_condition2), show(_show) {

		if (lstrcmpi(_evaluator.AsString(), "equals") == 0 || lstrcmpi(_evaluator.AsString(), "=") == 0 || lstrcmpi(_evaluator.AsString(), "==") == 0)
			evaluator = EQUALS;
		if (lstrcmpi(_evaluator.AsString(), "greaterthan") == 0 || lstrcmpi(_evaluator.AsString(), ">") == 0)
			evaluator = GREATERTHAN;
		if (lstrcmpi(_evaluator.AsString(), "lessthan") == 0 || lstrcmpi(_evaluator.AsString(), "<") == 0)
			evaluator = LESSTHAN;
		if (!evaluator)
			env->ThrowError("ConditionalFilter: Evaluator could not be recognized!");

		VideoInfo vi1 = source1->GetVideoInfo();
		VideoInfo vi2 = source2->GetVideoInfo();

		if (vi1.height != vi2.height)
			env->ThrowError("ConditionalFilter: The two sources must have the same height!");
		if (vi1.width != vi2.width)
			env->ThrowError("ConditionalFilter: The two sources must have the same width!");
		if (vi1.pixel_type != vi2.pixel_type)   // FIXME:  We need I420 -> YV12 transparency here!
			env->ThrowError("ConditionalFilter: The two sources must be the same colorspace!");

		vi.height = vi1.height;
		vi.width = vi1.width;
		vi.pixel_type = vi1.pixel_type;

	}

const char* t_TRUE="TRUE"; 
const char* t_FALSE="FALSE";


PVideoFrame __stdcall ConditionalFilter::GetFrame(int n, IScriptEnvironment* env) {

	env->SetVar("last",(AVSValue)child);			 // Set explicit last
	env->SetVar("current_frame",(AVSValue)n);  // Set frame to be tested by the conditional filters.

	ScriptParser parser(env, eval1.AsString(), "[Conditional Filter]");
	PExpression exp = parser.Parse();
	AVSValue e1_result = exp->Evaluate(env);

	ScriptParser parser2(env, eval2.AsString(), "[Conditional Filter]");
	exp = parser2.Parse();
	AVSValue e2_result = exp->Evaluate(env);

	int test_int=false;

	int e1 = 0;
	int e2 = 0;
	float f1 = 0.0f;
	float f2 = 0.0f;

	if (e1_result.IsInt() || e1_result.IsBool()) {
		test_int = true;
		e1 = e1_result.IsInt() ? e1_result.AsInt() : e1_result.AsBool();
		if (!(e2_result.IsInt() || e2_result.IsBool()))
			env->ThrowError("Conditional filter: Second expression did not return an integer or bool, as first expression.");
		e2 = e2_result.IsInt() ? e2_result.AsInt() : e2_result.AsBool();

	} else if (e1_result.IsFloat()) {
		f1 = e1_result.AsFloat();
		if (!e1_result.IsFloat()) 
			env->ThrowError("Conditional filter: Second expression did not return a float or an integer, as first expression.");
		f2 = e2_result.AsFloat();
	} else {
		env->ThrowError("ConditionalFilter: First condition did not return an int, bool or float!");
	}


	bool state = false;

	if (test_int) {
		if (evaluator&EQUALS) 
			if (e1 == e2) state = true;

		if (evaluator&GREATERTHAN) 
			if (e1 > e2) state = true;

		if (evaluator&LESSTHAN) 
			if (e1 < e2) state = true;
	} else {  // Float compare
		if (evaluator&EQUALS) 
			if (fabs(f1-f2)<0.000001f) state = true;   // Exact equal will sometimes be rounded to wrong values.

		if (evaluator&GREATERTHAN) 
			if (f1 > f2) state = true;

		if (evaluator&LESSTHAN) 
			if (f1 < f2) state = true;		
	}

	if (show) {
      char text[400];
			if (test_int) {
				sprintf(text,
					"Left side Conditional Result:%i\n"
					"Right side Conditional Result:%i\n"
					"Evaluate result: %s\n",
					e1, e2, (state) ? t_TRUE : t_FALSE
				);
			} else {
				sprintf(text,
					"Left side Conditional Result:%7.4f\n"
					"Right side Conditional Result:%7.4f\n"
					"Evaluate result: %s\n", 
					f1, f2, (state) ? t_TRUE : t_FALSE
				);
			}

			PVideoFrame dst = (state) ? source1->GetFrame(n,env) : source2->GetFrame(n,env);
			env->MakeWritable(&dst);
			ApplyMessage(&dst, vi, text, vi.width/4, 0xa0a0a0,0,0 , env );

		return dst;
	}

	if (state) 
		return source1->GetFrame(n,env);
	
	return source2->GetFrame(n,env);
}


AVSValue __cdecl ConditionalFilter::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new ConditionalFilter(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), args[3], args[4], args[5], args[6].AsBool(false), env);
}

