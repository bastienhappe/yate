/**
 * jsobject.cpp
 * Yet Another (Java)script library
 * This file is part of the YATE Project http://YATE.null.ro
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2011 Null Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "yatescript.h"

using namespace TelEngine;

namespace { // anonymous

// Object object
class JsObjectObj : public JsObject
{
    YCLASS(JsObjectObj,JsObject)
public:
    inline JsObjectObj(Mutex* mtx)
	: JsObject("Object",mtx,true)
	{
	}
protected:
    bool runNative(ObjList& stack, const ExpOperation& oper, GenObject* context);
};

// Date object
class JsDate : public JsObject
{
    YCLASS(JsDate,JsObject)
public:
    inline JsDate(Mutex* mtx)
	: JsObject("Date",mtx,true)
	{
	    params().addParam(new ExpFunction("now"));
	    params().addParam(new ExpFunction("getDate"));
	    params().addParam(new ExpFunction("getDay"));
	    params().addParam(new ExpFunction("getFullYear"));
	    params().addParam(new ExpFunction("getHours"));
	    params().addParam(new ExpFunction("getMilliseconds"));
	    params().addParam(new ExpFunction("getMinutes"));
	    params().addParam(new ExpFunction("getMonth"));
	    params().addParam(new ExpFunction("getSeconds"));
	    params().addParam(new ExpFunction("getTime"));

	    params().addParam(new ExpFunction("getUTCDate"));
	    params().addParam(new ExpFunction("getUTCDay"));
	    params().addParam(new ExpFunction("getUTCFullYear"));
	    params().addParam(new ExpFunction("getUTCHours"));
	    params().addParam(new ExpFunction("getUTCMilliseconds"));
	    params().addParam(new ExpFunction("getUTCMinutes"));
	    params().addParam(new ExpFunction("getUTCMonth"));
	    params().addParam(new ExpFunction("getUTCSeconds"));
	}
protected:
    inline JsDate(Mutex* mtx, const char* name)
	: JsObject(mtx,name)
	{ }
    virtual JsObject* clone(const char* name) const
	{ return new JsDate(mutex(),name); }
    bool runNative(ObjList& stack, const ExpOperation& oper, GenObject* context);
};

// Math class - not really an object, all methods are static
class JsMath : public JsObject
{
    YCLASS(JsMath,JsObject)
public:
    inline JsMath(Mutex* mtx)
	: JsObject("Math",mtx,true)
	{
	    params().addParam(new ExpFunction("abs"));
	    params().addParam(new ExpFunction("max"));
	    params().addParam(new ExpFunction("min"));
	}
protected:
    bool runNative(ObjList& stack, const ExpOperation& oper, GenObject* context);
};

}; // anonymous namespace


// Helper function that does the actual object printing
static void dumpRecursiveObj(const GenObject* obj, String& buf, unsigned int depth, ObjList& seen)
{
    if (!obj)
	return;
    String str(' ',2 * depth);
    if (seen.find(obj)) {
	str << "(recursivity encountered)";
	buf.append(str,"\r\n");
	return;
    }
    const NamedString* nstr = YOBJECT(NamedString,obj);
    const NamedPointer* nptr = YOBJECT(NamedPointer,nstr);
    const char* type = nstr ? (nptr ? "NamedPointer" : "NamedString") : "???";
    const ScriptContext* scr = YOBJECT(ScriptContext,obj);
    const ExpWrapper* wrap = 0;
    bool objRecursed = false;
    if (scr) {
	const JsObject* jso = YOBJECT(JsObject,scr);
	if (jso) {
	    objRecursed = (seen.find(jso) != 0);
	    if ((jso != obj) && !objRecursed)
		seen.append(jso)->setDelete(false);
	    if (YOBJECT(JsArray,scr))
		type = "JsArray";
	    else if (YOBJECT(JsFunction,scr))
		type = "JsFunction";
	    else if (YOBJECT(JsRegExp,scr))
		type = "JsRegExp";
	    else
		type = "JsObject";
	}
	else
	    type = "ScriptContext";
    }
    seen.append(obj)->setDelete(false);
    const ExpOperation* exp = YOBJECT(ExpOperation,nstr);
    if (exp && !scr) {
	if ((wrap = YOBJECT(ExpWrapper,exp)))
	    type = wrap->object() ? "ExpWrapper" : "Undefined";
	else if (YOBJECT(ExpFunction,exp))
	    type = "ExpFunction";
	else
	    type = "ExpOperation";
    }
    if (nstr)
	str << "'" << nstr->name() << "' = '" << *nstr << "'";
    else
	str << "'" << obj->toString() << "'";
    str << " (" << type << ")";
    if (objRecursed)
	str << " (already seen)";
    buf.append(str,"\r\n");
    if (objRecursed)
	return;
    str.clear();
    if (scr) {
	NamedIterator iter(scr->params());
	while (const NamedString* p = iter.get())
	    dumpRecursiveObj(p,buf,depth + 1,seen);
	if (scr->nativeParams()) {
	    iter = *scr->nativeParams();
	    while (const NamedString* p = iter.get())
		dumpRecursiveObj(p,buf,depth + 1,seen);
	}
    }
    else if (wrap)
	dumpRecursiveObj(wrap->object(),buf,depth + 1,seen);
    else if (nptr)
	dumpRecursiveObj(nptr->userData(),buf,depth + 1,seen);
}


const String JsObject::s_protoName("__proto__");

JsObject::JsObject(const char* name, Mutex* mtx, bool frozen)
    : ScriptContext(String("[object ") + name + "]"),
      m_frozen(frozen), m_mutex(mtx)
{
    XDebug(DebugAll,"JsObject::JsObject('%s',%p,%s) [%p]",
	name,mtx,String::boolText(frozen),this);
    params().addParam(new ExpFunction("freeze"));
    params().addParam(new ExpFunction("isFrozen"));
    params().addParam(new ExpFunction("toString"));
    params().addParam(new ExpFunction("hasOwnProperty"));
}

JsObject::JsObject(Mutex* mtx, const char* name, bool frozen)
    : ScriptContext(name),
      m_frozen(frozen), m_mutex(mtx)
{
    XDebug(DebugAll,"JsObject::JsObject(%p,'%s',%s) [%p]",
	mtx,name,String::boolText(frozen),this);
}

JsObject::~JsObject()
{
    XDebug(DebugAll,"JsObject::~JsObject '%s' [%p]",toString().c_str(),this);
}

void JsObject::dumpRecursive(const GenObject* obj, String& buf)
{
    ObjList seen;
    dumpRecursiveObj(obj,buf,0,seen);
}

void JsObject::printRecursive(const GenObject* obj)
{
    String buf;
    dumpRecursive(obj,buf);
    Output("%s",buf.c_str());
}

JsObject* JsObject::buildCallContext(Mutex* mtx, ExpOperation* thisObj)
{
    JsObject* ctxt = new JsObject(mtx,"()");
    if (thisObj)
	ctxt->params().addParam(new ExpWrapper(thisObj,"this"));
    return ctxt;
}

void JsObject::fillFieldNames(ObjList& names)
{
    ScriptContext::fillFieldNames(names,params(),"__");
    const NamedList* native = nativeParams();
    if (native)
	ScriptContext::fillFieldNames(names,*native);
}

bool JsObject::hasField(ObjList& stack, const String& name, GenObject* context) const
{
    if (ScriptContext::hasField(stack,name,context))
	return true;
    const ScriptContext* proto = YOBJECT(ScriptContext,params().getParam(protoName()));
    if (proto && proto->hasField(stack,name,context))
	return true;
    NamedList* np = nativeParams();
    return np && np->getParam(name);
}

NamedString* JsObject::getField(ObjList& stack, const String& name, GenObject* context) const
{
    NamedString* fld = ScriptContext::getField(stack,name,context);
    if (fld)
	return fld;
    const ScriptContext* proto = YOBJECT(ScriptContext,params().getParam(protoName()));
    if (proto) {
	fld = proto->getField(stack,name,context);
	if (fld)
	    return fld;
    }
    NamedList* np = nativeParams();
    if (np)
	return np->getParam(name);
    return 0;
}

JsObject* JsObject::runConstructor(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    if (!ref())
	return 0;
    JsObject* obj = clone();
    obj->params().addParam(new ExpWrapper(this,protoName()));
    return obj;
}

bool JsObject::runFunction(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    XDebug(DebugInfo,"JsObject::runFunction() '%s' in '%s' [%p]",
	oper.name().c_str(),toString().c_str(),this);
    NamedString* param = getField(stack,oper.name(),context);
    if (!param)
	return false;
    ExpFunction* ef = YOBJECT(ExpFunction,param);
    if (ef)
	return runNative(stack,oper,context);
    JsFunction* jf = YOBJECT(JsFunction,param);
    if (jf)
	return jf->runDefined(stack,oper,context);
    return false;
}

bool JsObject::runField(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    XDebug(DebugAll,"JsObject::runField() '%s' in '%s' [%p]",
	oper.name().c_str(),toString().c_str(),this);
    const String* param = getField(stack,oper.name(),context);
    if (param) {
	ExpFunction* ef = YOBJECT(ExpFunction,param);
	if (ef)
	    ExpEvaluator::pushOne(stack,ef->ExpOperation::clone());
	else {
	    JsFunction* jf = YOBJECT(JsFunction,param);
	    if (jf)
		ExpEvaluator::pushOne(stack,new ExpFunction(oper.name()));
	    else {
		ExpWrapper* w = YOBJECT(ExpWrapper,param);
		if (w)
		    ExpEvaluator::pushOne(stack,w->clone(oper.name()));
		else
		    ExpEvaluator::pushOne(stack,new ExpOperation(*param,oper.name(),true));
	    }
	}
    }
    else
	ExpEvaluator::pushOne(stack,new ExpWrapper(0,oper.name()));
    return true;
}

bool JsObject::runAssign(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    XDebug(DebugAll,"JsObject::runAssign() '%s'='%s' in '%s' [%p]",
	oper.name().c_str(),oper.c_str(),toString().c_str(),this);
    if (frozen()) {
	Debug(DebugNote,"Object '%s' is frozen",toString().c_str());
	return false;
    }
    ExpFunction* ef = YOBJECT(ExpFunction,&oper);
    if (ef)
	params().setParam(ef->ExpOperation::clone());
    else {
	ExpWrapper* w = YOBJECT(ExpWrapper,&oper);
	if (w) {
	    GenObject* o = w->object();
	    if (o)
		params().setParam(w->clone(oper.name()));
	    else
		params().clearParam(oper.name());
	}
	else
	    params().setParam(new NamedString(oper.name(),oper));
    }
    return true;
}

bool JsObject::runNative(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    XDebug(DebugAll,"JsObject::runNative() '%s' in '%s' [%p]",
	oper.name().c_str(),toString().c_str(),this);
    if (oper.name() == YSTRING("freeze"))
	freeze();
    else if (oper.name() == YSTRING("isFrozen"))
	ExpEvaluator::pushOne(stack,new ExpOperation(frozen()));
    else if (oper.name() == YSTRING("toString"))
	ExpEvaluator::pushOne(stack,new ExpOperation(params()));
    else if (oper.name() == YSTRING("hasOwnProperty")) {
	bool ok = true;
	for (long int i = oper.number(); i; i--) {
	    ExpOperation* op = popValue(stack,context);
	    if (!op)
		continue;
	    ok = ok && params().getParam(*op);
	    TelEngine::destruct(op);
	}
	ExpEvaluator::pushOne(stack,new ExpOperation(ok));
    }
    else
	return false;
    return true;
}

ExpOperation* JsObject::popValue(ObjList& stack, GenObject* context)
{
    ExpOperation* oper = ExpEvaluator::popOne(stack);
    if (!oper || (oper->opcode() != ExpEvaluator::OpcField))
	return oper;
    XDebug(DebugAll,"JsObject::popValue() field '%s' in '%s' [%p]",
	oper->name().c_str(),toString().c_str(),this);
    bool ok = runMatchingField(stack,*oper,context);
    TelEngine::destruct(oper);
    return ok ? ExpEvaluator::popOne(stack) : 0;
}

// Static method that adds an object to a parent
void JsObject::addObject(NamedList& params, const char* name, JsObject* obj)
{
    params.addParam(new NamedPointer(name,obj,obj->toString()));
}

// Static method that adds a constructor to a parent
void JsObject::addConstructor(NamedList& params, const char* name, JsObject* obj)
{
    JsFunction* ctr = new JsFunction(obj->mutex(),name);
    ctr->params().addParam(new NamedPointer("prototype",obj,obj->toString()));
    obj->initConstructor(ctr);
    params.addParam(new NamedPointer(name,ctr,ctr->toString()));
}

// Static method that pops arguments off a stack to a list in proper order
int JsObject::extractArgs(JsObject* obj, ObjList& stack, const ExpOperation& oper,
    GenObject* context, ObjList& arguments)
{
    if (!obj || !oper.number())
	return 0;
    for (long int i = oper.number(); i;  i--) {
	ExpOperation* op = obj->popValue(stack,context);
	arguments.insert(op);
    }
    return oper.number();
}

// Initialize standard globals in the execution context
void JsObject::initialize(ScriptContext* context)
{
    if (!context)
	return;
    Mutex* mtx = context->mutex();
    Lock mylock(mtx);
    NamedList& p = context->params();
    static_cast<String&>(p) = "[object Global]";
    if (!p.getParam(YSTRING("Object")))
	addConstructor(p,"Object",new JsObjectObj(mtx));
    if (!p.getParam(YSTRING("Function")))
	addConstructor(p,"Function",new JsFunction(mtx));
    if (!p.getParam(YSTRING("Array")))
	addConstructor(p,"Array",new JsArray(mtx));
    if (!p.getParam(YSTRING("RegExp")))
	addConstructor(p,"RegExp",new JsRegExp(mtx));
    if (!p.getParam(YSTRING("Date")))
	addConstructor(p,"Date",new JsDate(mtx));
    if (!p.getParam(YSTRING("Math")))
	addObject(p,"Math",new JsMath(mtx));
}


bool JsObjectObj::runNative(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    if (oper.name() == YSTRING("constructor"))
	ExpEvaluator::pushOne(stack,new ExpWrapper(new JsObject("Object",mutex())));
    else
	return JsObject::runNative(stack,oper,context);
    return true;
}


JsArray::JsArray(Mutex* mtx)
    : JsObject("Array",mtx), m_length(0)
{
    params().addParam(new ExpFunction("push"));
    params().addParam(new ExpFunction("pop"));
    params().addParam(new ExpFunction("concat"));
    params().addParam(new ExpFunction("join"));
    params().addParam(new ExpFunction("reverse"));
    params().addParam(new ExpFunction("shift"));
    params().addParam(new ExpFunction("unshift"));
    params().addParam(new ExpFunction("slice"));
    params().addParam(new ExpFunction("splice"));
    params().addParam(new ExpFunction("sort"));
    params().addParam("length","0");
}

void JsArray::push(ExpOperation* item)
{
    if (!item)
	return;
    unsigned int pos = m_length;
    while (params().getParam(String(pos)))
	pos++;
    const_cast<String&>(item->name()) = pos;
    params().addParam(item);
    setLength(pos + 1);
}

bool JsArray::runNative(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    XDebug(DebugAll,"JsArray::runNative() '%s' in '%s' [%p]",
	oper.name().c_str(),toString().c_str(),this);
    if (oper.name() == YSTRING("push")) {
	// Adds one or more elements to the end of an array and returns the new length of the array.
	if (!oper.number())
	    return false;
	for (long int i = oper.number(); i; i--) {
	    ExpOperation* op = popValue(stack,context);
	    ExpWrapper* obj = YOBJECT(ExpWrapper,op);
	    if (!obj)
		continue;
	    JsObject* jo = (JsObject*)obj->getObject(YSTRING("JsObject"));
	    if (!jo)
		continue;
	    jo->ref();
	    params().addParam(new NamedPointer(String((unsigned int)(m_length + i - 1)),jo));
	    TelEngine::destruct(op);
	}
	m_length += oper.number();
	setLength();
	ExpEvaluator::pushOne(stack,new ExpOperation(length()));
    }
    else if (oper.name() == YSTRING("pop")) {
	// Removes the last element from an array and returns that element
	if (m_length < 1)
	    ExpEvaluator::pushOne(stack,new ExpWrapper(0,0));

	NamedString* last = 0;
	while (!last) {
	    last = params().getParam(String((int)--m_length));
	    if (m_length == 0)
		break;
	}
	if (!last)
	    ExpEvaluator::pushOne(stack,new ExpWrapper(0,0));
	else {
	    NamedPointer* np = (NamedPointer*)last->getObject(YSTRING("NamedPointer"));
	    if (!np)
		ExpEvaluator::pushOne(stack,new ExpOperation(last->toString()));
	    else
		ExpEvaluator::pushOne(stack,new ExpWrapper(np->userData(),0));
	}
	// clear last
	params().clearParam(last);
	setLength();
    }
    else if (oper.name() == YSTRING("length")) {
	// Reflects the number of elements in an array.
	ExpEvaluator::pushOne(stack,new ExpOperation(length()));
    }
    else if (oper.name() == YSTRING("concat")) {
	// Returns a new array comprised of this array joined with other array(s) and/or value(s).
	// var num1 = [1, 2, 3];  
	// var num2 = [4, 5, 6];  
	// var num3 = [7, 8, 9];  
	//
	// creates array [1, 2, 3, 4, 5, 6, 7, 8, 9]; num1, num2, num3 are unchanged  
	// var nums = num1.concat(num2, num3);  

	// var alpha = ['a', 'b', 'c'];  
	// creates array ["a", "b", "c", 1, 2, 3], leaving alpha unchanged  
	// var alphaNumeric = alpha.concat(1, [2, 3]);
	if (!oper.number())
	    return false;
	JsArray* array = new JsArray(mutex());
	// copy this array
	for (long int i = 0; i < m_length; i++)
	    array->params().addParam(params().getParam(String((int)i)));
	array->setLength(length());
	// add parameters (JsArray of JsObject)
	for (long int i = oper.number(); i; i--) {
	    ExpOperation* op = popValue(stack,context);
	    ExpWrapper* obj = YOBJECT(ExpWrapper,op);
	    if (!obj)
		continue;
	    JsArray* ja = (JsArray*)obj->getObject(YSTRING("JsArray"));
	    if (ja) {
		for (long int i = 0; i < ja->length(); i++)
		    array->params().addParam(String((int)(i + array->length())),ja->params().getValue(String((int)i)));
		array->setLength(array->length() + ja->length());
	    }
	    else {
		JsObject* jo = (JsObject*)obj->getObject(YSTRING("JsObject"));
		if (jo) {
		    array->params().addParam(new NamedPointer(String((unsigned int)array->length()),jo));
		    array->setLength(array->length() + 1);
		    jo->ref();
		}
		else
		    continue;
	    }
	    TelEngine::destruct(op);
	}
	ExpEvaluator::pushOne(stack,new ExpWrapper(array,0));
    }
    else if (oper.name() == YSTRING("join")) {
	// Joins all elements of an array into a string
	// var a = new Array("Wind","Rain","Fire");
	// var myVar1 = a.join();      // assigns "Wind,Rain,Fire" to myVar1
	// var myVar2 = a.join(", ");  // assigns "Wind, Rain, Fire" to myVar2
	// var myVar3 = a.join(" + "); // assigns "Wind + Rain + Fire" to myVar3
	String separator = ",";
	if (oper.number()) {
	    ExpOperation* op = popValue(stack,context);
	    separator = op->toString();
	}
	String result;
	for (long int i = 0; i < length(); i++)
	    result.append(params()[String((int)i)],separator);
	ExpEvaluator::pushOne(stack,new ExpOperation(result));
    }
    else if (oper.name() == YSTRING("reverse")) {
	// Reverses the order of the elements of an array -- the first becomes the last, and the last becomes the first.
	// var myArray = ["one", "two", "three"];
	// myArray.reverse(); => three, two, one
	NamedList reversed("");
	String separator = ",";
	String toCopy;
	for (long int i = 0; i < length(); i++)
	    toCopy.append(params()[String((int)i)],separator);
	reversed.copyParams(params(),toCopy);
	for (long int i = length(); i; i--)
	    params().setParam(String((int)(length() - i)),reversed.getValue(String((int)(i - 1))));
    }
    else if (oper.name() == YSTRING("shift")) {
	// Removes the first element from an array and returns that element
	// var myFish = ["angel", "clown", "mandarin", "surgeon"];
	// println("myFish before: " + myFish);
	// var shifted = myFish.shift();
	// println("myFish after: " + myFish);
	// println("Removed this element: " + shifted);
	// This example displays the following:

	// myFish before: angel,clown,mandarin,surgeon
	// myFish after: clown,mandarin,surgeon
	// Removed this element: angel
	if (!length())
	    ExpEvaluator::pushOne(stack,new ExpWrapper(0,0));
	else {
	    ExpEvaluator::pushOne(stack,new ExpOperation(params().getValue("0")));
	    // shift : value n+1 becomes value n
	    for (long int i = 0; i < length() - 1; i++)
		params().setParam(String((int)i),params().getValue(String((int)i + 1)));
	    params().clearParam(String((int)(length() - 1)));
	    setLength(length() - 1);
	}
    }
    else if (oper.name() == YSTRING("unshift")) {
	// Adds one or more elements to the front of an array and returns the new length of the array
	// myFish = ["angel", "clown"];
	// println("myFish before: " + myFish);
	// unshifted = myFish.unshift("drum", "lion");
	// println("myFish after: " + myFish);
	// println("New length: " + unshifted);
	// This example displays the following:
	// myFish before: ["angel", "clown"]
	// myFish after: ["drum", "lion", "angel", "clown"]
	// New length: 4
	// shift array
	long shift = oper.number();
	for (long int i = length(); i; i--)
	    params().setParam(String((int)(i - 1 + shift)),params().getValue(String((int)(i - 1))));
	for (long int i = shift; i; i--) {
	    ExpOperation* op = popValue(stack,context);
	    ExpWrapper* obj = YOBJECT(ExpWrapper,op);
	    if (!obj)
		continue;
	    JsObject* jo = (JsObject*)obj->getObject(YSTRING("JsObject"));
	    if (!jo)
		continue;
	    jo->ref();
	    params().clearParam(String((int)(i - 1)));
	    params().setParam(new NamedPointer(String((int)(i - 1)),jo));
	    TelEngine::destruct(op);
	}
	setLength(length() + shift);
	ExpEvaluator::pushOne(stack,new ExpOperation(length()));
    }
    else if (oper.name() == YSTRING("slice"))
	return runNativeSlice(stack,oper,context);
    else if (oper.name() == YSTRING("splice"))
	return runNativeSplice(stack,oper,context);
    else if (oper.name() == YSTRING("sort")) {
	return runNativeSort(stack,oper,context);
    }
    else if (oper.name() == YSTRING("toString")) {
	// Override the JsObject toString method
	// var monthNames = ['Jan', 'Feb', 'Mar', 'Apr'];
	// var myVar = monthNames.toString(); // assigns "Jan,Feb,Mar,Apr" to myVar.
	String separator = ",";
	String result;
	for (long int i = 0; i < length(); i++)
	    result.append(params()[String((int)i)],separator);
	ExpEvaluator::pushOne(stack,new ExpOperation(result));
    }
    else
	return JsObject::runNative(stack,oper,context);
    return true;
}

bool JsArray::runNativeSlice(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    // Extracts a section of an array and returns a new array.
    // var myHonda = { color: "red", wheels: 4, engine: { cylinders: 4, size: 2.2 } };  
    // var myCar = [myHonda, 2, "cherry condition", "purchased 1997"];  
    // var newCar = myCar.slice(0, 2); 
    if (!oper.number())
	return false;
    // begin | end > 0 offset from the start of the array
    //	       < 0 offset from the end of the array
    // end missing -> go to end of array  
    int begin = length(), end = length();
    for (long int i = oper.number(); i; i--) {
	ExpOperation* op = popValue(stack,context);
	if (op->isInteger()) {
	    end = begin;
	    begin = op->number();
	}
	TelEngine::destruct(op);
    }
    if (begin < 0)
	begin = length() + begin;
    if (end < 0)
	end = length() + end;
    if (end < begin)
	return false;
    // TODO
    //JsArray* slice = new JsArray(mutex());
    for (long int i = begin; i < end; i++) {
//	slice->params().addParam(new NamedString(String((int)(i - begin),
    }
    return true;
}

bool JsArray::runNativeSplice(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    // Changes the content of an array, adding new elements while removing old elements. 
    // Returns an array containing the removed elements
    // array.splice(index , howMany[, element1[, ...[, elementN]]])
    // array.splice(index ,[ howMany[, element1[, ...[, elementN]]]])
    ObjList arguments;
    int argc = extractArgs(this,stack,oper,context,arguments);
    if (!argc)
	return false;
    // get start index
    ExpOperation* op = static_cast<ExpOperation*>(arguments[0]);
    int begin = op->number();
    if (begin < 0)
	begin = length() + begin;
    // get count to delete
    int count = length() - begin;
    if (arguments.count() > 1) {
	// get count
	op = static_cast<ExpOperation*>(arguments[1]);
	count = op->number();
    }

    // remove elements
    JsArray* removed = new JsArray(mutex());
    for (int i = begin; i < begin + count; i++) {
	removed->params().setParam(String(begin + count - begin),params().getValue(String(i)));
	params().clearParam(String(i));
    }
    removed->setLength(count);

    // add the trailing array elements
    // index for trailing array elements arfer removing the specified count
    int shiftIdx = begin + count;
    // shift how many positions for trailing array elements
    int shiftWith = arguments.count() > 2 ? arguments.count() - 2 - count : -count;
    if (shiftWith > 0) {
	// shift everything starting at index shiftIdx with shiftWith positions to the right
	for (int i = length(); i > shiftIdx; i--)
	    params().setParam(String(i - 1 + shiftWith),params().getValue(String(i - 1)));
    }
    else if (shiftWith < 0) {
	// shift everything starting at index shiftIdx with shiftWith positions to the left
	for (int i = shiftIdx; i < length(); i++)
	    params().setParam(String(i + shiftWith),params().getValue(String(i)));
    }
    // insert the new ones 
    for (int i = begin;(arguments.count() > 2) && (i < length()); i++) {
	GenObject* obj = arguments[2 + i - begin];
	params().setParam(new NamedPointer(String(i),obj));
    }
    // set length
    setLength(arguments.count() > 2 ? length() + arguments.count() - 2 - count : length() - count);
    // push the removed array on stack
    ExpEvaluator::pushOne(stack,new ExpWrapper(removed,0));
    return true;
}

bool JsArray::runNativeSort(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    // TODO
    return false;
}


JsRegExp::JsRegExp(Mutex* mtx)
    : JsObject("RegExp",mtx)
{
    params().addParam(new ExpFunction("test"));
}

JsRegExp::JsRegExp(Mutex* mtx, const char* name, const char* rexp, bool insensitive, bool extended, bool frozen)
    : JsObject(mtx,name,frozen),
      m_regexp(rexp,extended,insensitive)
{
    params().addParam(new ExpFunction("test"));
    params().addParam("ignoreCase",String::boolText(insensitive));
    params().addParam("basicPosix",String::boolText(!extended));
}

bool JsRegExp::runNative(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    XDebug(DebugAll,"JsRegExp::runNative() '%s' in '%s' [%p]",
	oper.name().c_str(),toString().c_str(),this);
    if (oper.name() == YSTRING("test")) {
	if (oper.number() != 1)
	    return false;
	ExpOperation* op = popValue(stack,context);
	bool ok = op && regexp().matches(*op);
	TelEngine::destruct(op);
	ExpEvaluator::pushOne(stack,new ExpOperation(ok));
    }
    else
	return JsObject::runNative(stack,oper,context);
    return true;
}


bool JsMath::runNative(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    XDebug(DebugAll,"JsMath::runNative() '%s' in '%s' [%p]",
	oper.name().c_str(),toString().c_str(),this);
    if (oper.name() == YSTRING("abs")) {
	if (!oper.number())
	    return false;
	long int n = 0;
	for (long int i = oper.number(); i; i--) {
	    ExpOperation* op = popValue(stack,context);
	    if (op->isInteger())
		n = op->number();
	    TelEngine::destruct(op);
	}
	if (n < 0)
	    n = -n;
	ExpEvaluator::pushOne(stack,new ExpOperation(n));
    }
    else if (oper.name() == YSTRING("max")) {
	if (!oper.number())
	    return false;
	long int n = LONG_MIN;
	for (long int i = oper.number(); i; i--) {
	    ExpOperation* op = popValue(stack,context);
	    if (op->isInteger() && op->number() > n)
		n = op->number();
	    TelEngine::destruct(op);
	}
	ExpEvaluator::pushOne(stack,new ExpOperation(n));
    }
    else if (oper.name() == YSTRING("min")) {
	if (!oper.number())
	    return false;
	long int n = LONG_MAX;
	for (long int i = oper.number(); i; i--) {
	    ExpOperation* op = popValue(stack,context);
	    if (op->isInteger() && op->number() < n)
		n = op->number();
	    TelEngine::destruct(op);
	}
	ExpEvaluator::pushOne(stack,new ExpOperation(n));
    }
    else
	return JsObject::runNative(stack,oper,context);
    return true;
}


bool JsDate::runNative(ObjList& stack, const ExpOperation& oper, GenObject* context)
{
    XDebug(DebugAll,"JsDate::runNative() '%s' in '%s' [%p]",
	oper.name().c_str(),toString().c_str(),this);
    if (oper.name() == YSTRING("now")) {
	// Returns the number of milliseconds elapsed since 1 January 1970 00:00:00 UTC.
	ExpEvaluator::pushOne(stack,new ExpOperation((long int)Time::msecNow()));  // should check conversion from u_int64_t 
    }
    else if (oper.name() == YSTRING("getDate")) {
	// Returns the day of the month for the specified date according to local time.
	// The value returned by getDate is an integer between 1 and 31.
	// !NOTE : assuming that the date for this object is kept in params()
	unsigned int time = params().getIntValue("time");
	int year = 0;
	unsigned int month = 0, day = 0, hour = 0, minute = 0, sec = 0;
	if (Time::toDateTime(time,year,month,day,hour,minute,sec))
	    ExpEvaluator::pushOne(stack,new ExpOperation((long int)day));
	else
	    return false;
    }
    else if (oper.name() == YSTRING("getDay")) {
	// Get the day of the week for the date (0 is Sunday and returns values 0-6)
	// TODO
    }
    else if (oper.name() == YSTRING("getFullYear")) {
	// Returns the year of the specified date according to local time.
	// !NOTE : assuming that the date for this object is kept in params()
	unsigned int time = params().getIntValue("time");
	int year = 0;
	unsigned int month = 0, day = 0, hour = 0, minute = 0, sec = 0;
	if (Time::toDateTime(time,year,month,day,hour,minute,sec))
	    ExpEvaluator::pushOne(stack,new ExpOperation((long int)year));
	else
	    return false;
    }
    else if (oper.name() == YSTRING("getHours")) {
	// Returns the hour ( 0 - 23) of the specified date according to local time.
	// !NOTE : assuming that the date for this object is kept in params()
	unsigned int time = params().getIntValue("time");
	int year = 0;
	unsigned int month = 0, day = 0, hour = 0, minute = 0, sec = 0;
	if (Time::toDateTime(time,year,month,day,hour,minute,sec))
	    ExpEvaluator::pushOne(stack,new ExpOperation((long int)hour));
	else
	    return false;
    }
    else if (oper.name() == YSTRING("getMilliseconds")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getMinutes")) {
	// Returns the minute ( 0 - 59 ) of the specified date according to local time.
	// !NOTE : assuming that the date for this object is kept in params()
	unsigned int time = params().getIntValue("time");
	int year = 0;
	unsigned int month = 0, day = 0, hour = 0, minute = 0, sec = 0;
	if (Time::toDateTime(time,year,month,day,hour,minute,sec))
	    ExpEvaluator::pushOne(stack,new ExpOperation((long int)minute));
	else
	    return false;
    }
    else if (oper.name() == YSTRING("getMonth")) {
	// Returns the minute ( 0 - 11 ) of the specified date according to local time.
	// !NOTE : assuming that the date for this object is kept in params()
	unsigned int time = params().getIntValue("time");
	int year = 0;
	unsigned int month = 0, day = 0, hour = 0, minute = 0, sec = 0;
	if (Time::toDateTime(time,year,month,day,hour,minute,sec))
	    ExpEvaluator::pushOne(stack,new ExpOperation((long int)month - 1));
	else
	    return false;
    }
    else if (oper.name() == YSTRING("getSeconds")) {
	// Returns the second ( 0 - 59 ) of the specified date according to local time.
	// !NOTE : assuming that the date for this object is kept in params()
	unsigned int time = params().getIntValue("time");
	int year = 0;
	unsigned int month = 0, day = 0, hour = 0, minute = 0, sec = 0;
	if (Time::toDateTime(time,year,month,day,hour,minute,sec))
	    ExpEvaluator::pushOne(stack,new ExpOperation((long int)sec));
	else
	    return false;
    }
    else if (oper.name() == YSTRING("getTime")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getUTCDate")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getUTCDay")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getUTCFullYear")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getUTCHours")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getUTCMilliseconds")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getUTCMinutes")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getUTCMonth")) {
	// TODO
    }
    else if (oper.name() == YSTRING("getUTCSeconds")) {
	// TODO
    }
    else
	return JsObject::runNative(stack,oper,context);
    return true;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
