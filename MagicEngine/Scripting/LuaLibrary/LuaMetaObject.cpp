/*
   MIT License

   Copyright (c) 2021 Jordan Vrtanoski

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   */

#include <iostream>

#include "LuaMetaObject.hpp"
#include "Engine/LuaTNil.hpp"
#include "Engine/LuaTTable.hpp"
#include "Engine/LuaTString.hpp"
#include "Engine/LuaTNumber.hpp"
#include "Engine/LuaTBoolean.hpp"

using namespace LuaCpp;
using namespace LuaCpp::Engine;

LuaMetaObject::LuaMetaObject() : LuaTUserData(sizeof(LuaMetaObject *)) {
	AddMetaFunction("__index", u_index);
	AddMetaFunction("__newindex", u_newindex);
	AddMetaFunction("__call", u_call);
}

void LuaMetaObject::_storeData() {
	*((void **) userdata) = (void *) this;
}

void LuaMetaObject::_retreiveData() {
}

std::shared_ptr<LuaType> LuaMetaObject::getValue(int) {
	return std::make_shared<LuaTNil>();
}

std::shared_ptr<LuaType> LuaMetaObject::getValue(std::string &) {
	return std::make_shared<LuaTNil>();
}

void LuaMetaObject::setValue(int, std::shared_ptr<LuaType>) {
}

void LuaMetaObject::setValue(std::string &, std::shared_ptr<LuaType>) {
}

int LuaMetaObject::_getValue(LuaState &L) {
	if (lua_type(L, 2) == LUA_TSTRING) {
		std::string key(lua_tostring(L, 2));
		getValue(key)->PushValue(L);
	} else {
		getValue(static_cast<int>(lua_tointeger(L,2)))->PushValue(L);
	}
	return 1;
}


int LuaMetaObject::_setValue(LuaState &L) {
        std::shared_ptr<LuaType> val = std::make_shared<LuaTNil>();
	switch (lua_type(L, -1)) {
	    case LUA_TSTRING: {
		val = std::make_shared<LuaTString>("");
		val->PopValue(L, -1);
		break;
	    }  
	    case LUA_TTABLE: {
	        val = std::make_shared<LuaTTable>();
		val->PopValue(L, -1);
		break;
	    }
	    case LUA_TNUMBER: {
	        val = std::make_shared<LuaTNumber>(0);
		val->PopValue(L, -1);
		break;
            }
	    case LUA_TBOOLEAN: {
	        val = std::make_shared<LuaTBoolean>(false);
		val->PopValue(L, -1);
		break;
	    }
	    default: {
                val = std::make_shared<LuaTString>(lua_typename(L, lua_type(L, -1)));
		break;
            }
	}
	if (lua_type(L, 2) == LUA_TSTRING) {
		std::string key(lua_tostring(L, 2));
		setValue(key, val);
	} else {
		int key = static_cast<int>(lua_tointeger(L,2));
		setValue(key, val);
	}

	return 0;
}

int LuaMetaObject::Execute(LuaState &) {
	return 0;
}

static int LuaCpp::u_newindex(lua_State *L) {
	void * ud = lua_touserdata(L, 1);
	LuaState _L(L, true);
	int res = (**((LuaMetaObject **) ud))._setValue(_L);

	return res;
}

static int LuaCpp::u_index(lua_State *L) {
	void * ud = lua_touserdata(L, 1);
	LuaState _L(L, true);
	int res = (**((LuaMetaObject **) ud))._getValue(_L);

	return res;
}


static int LuaCpp::u_call(lua_State *L) {
	void * ud = lua_touserdata(L, 1);
	LuaState _L(L, true);
	int res = (**((LuaMetaObject **) ud)).Execute(_L);

	return res;
}
