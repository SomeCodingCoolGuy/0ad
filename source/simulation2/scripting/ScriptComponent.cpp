/* Copyright (C) 2020 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "ScriptComponent.h"

#include "simulation2/serialization/ISerializer.h"
#include "simulation2/serialization/IDeserializer.h"

CComponentTypeScript::CComponentTypeScript(const ScriptInterface& scriptInterface, JS::HandleValue instance) :
	m_ScriptInterface(scriptInterface), m_Instance(scriptInterface.GetGeneralJSContext(), instance)
{
}

void CComponentTypeScript::Init(const CParamNode& paramNode, entity_id_t ent)
{
	m_ScriptInterface.SetProperty(m_Instance, "entity", (int)ent, true, false);
	m_ScriptInterface.SetProperty(m_Instance, "template", paramNode, true, false);
	m_ScriptInterface.CallFunctionVoid(m_Instance, "Init");
}

void CComponentTypeScript::Deinit()
{
	m_ScriptInterface.CallFunctionVoid(m_Instance, "Deinit");
}

void CComponentTypeScript::HandleMessage(const CMessage& msg, bool global)
{
	ScriptRequest rq(m_ScriptInterface);

	const char* name = global ? msg.GetScriptGlobalHandlerName() : msg.GetScriptHandlerName();

	JS::RootedValue msgVal(rq.cx, msg.ToJSValCached(m_ScriptInterface));

	if (!m_ScriptInterface.CallFunctionVoid(m_Instance, name, msgVal))
		LOGERROR("Script message handler %s failed", name);
}

void CComponentTypeScript::Serialize(ISerializer& serialize)
{
	ScriptRequest rq(m_ScriptInterface);

	serialize.ScriptVal("comp", &m_Instance);
}

void CComponentTypeScript::Deserialize(const CParamNode& paramNode, IDeserializer& deserialize, entity_id_t ent)
{
	ScriptRequest rq(m_ScriptInterface);

	m_ScriptInterface.SetProperty(m_Instance, "entity", (int)ent, true, false);
	m_ScriptInterface.SetProperty(m_Instance, "template", paramNode, true, false);

	deserialize.ScriptObjectAssign("comp", m_Instance);
}
