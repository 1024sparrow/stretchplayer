#!/usr/bin/node

const
	META = './config.js',
	CLASSNAME = 'Configuration2',
	CLASSFILENAME = 'configuration2',
	TARGET_DIR = '.'
;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const
	src = require(META),
	fs = require('fs'),
	path = require('path')
;

var _modes = `
		Undefined = 0,`;
var modeResolving = '';
var paramIfs = `if (!strcmp(arg, "--config"))
			state = -1;`;
var stateCounter = [];
for (let o of src.modes){
	paramIfs += `
		else if (!strcmp(arg, "--${o.name}"));`;
}
for (let o of src.options){
	stateCounter.push({
		state: stateCounter.length + 1,
		type: o.type,
		name: o.name
	});
	paramIfs += `
		else if (!strcmp(arg, "--${o.name}"))
			state = ${stateCounter.length};`;
}
for (let o of src.modes){
	o.inEnumName = o.name[0].toUpperCase() + o.name.slice(1);
	_modes += `
		${o.inEnumName},`;
	if (modeResolving)
		modeResolving += '\n\t\telse ';
	else
		modeResolving += '\n\t\t';
	modeResolving += `if (!strcmp(arg, "--${o.name}")) cand = Mode::${o.inEnumName};`;

	for (const oInModeParams of o.options){
		stateCounter.push({
			state: stateCounter.length + 1,
			type: oInModeParams.type,
			target: `_data.${o.name}.${oInModeParams.name}`,
			name: oInModeParams.name
		});
		if (paramIfs){
			paramIfs += `
		else `;
		}
		else{
			paramIfs += `
		`;
		}
		paramIfs += `if (_mode == Mode::${o.inEnumName} && !strcmp(arg, "--${oInModeParams.name}"))
			state = ${stateCounter.length};`;
	}
}
var _asd = `if (state == 0)
			{
				// свободный аргумент (вне параметров)
				if (arg[0] == '-')
				{
					std::string errorDescr = "unknown key: \\"";
					errorDescr += arg;
					errorDescr += "\\"";
					return collectError(p_error, errorDescr);
				}
			}
			else
			{
				if (arg[0] == '-')
				{
					std::string errorDescr = "unexpected key: \\"";
					errorDescr += arg;
					errorDescr += "\\". Expected previous parameter value instead.";
					return collectError(p_error, errorDescr);
				}
				if (state == -1)
				{
					state = 0;
				}`;
while (stateCounter.length){
	let state = stateCounter.shift();
	_asd += `
				else if (state == ${state.state})
				{`;
	if (state.type === 'integer'){
		_asd += `
					int tmp = atoi(arg);`;
	}
	else if (state.type === 'string'){
		_asd += `
					const char *tmp = arg;`;
	}
	else if (state.type === 'boolean'){
		_asd += `
					bool tmp;
					if (!strcmp(arg, "true"))
						tmp = true;
					else if (!strcmp(arg, "false"))
						tmp = false;
					else
						return collectError(p_error, "--${state.name}: true|false expected");`;
	}
	else{
		console.log('incorrect parameter type pointed');
		process.exit(1);
	}
	if (state.target){
		_asd += `
					${state.target} = tmp;`;
	}
	else{
		for (const oMode of src.modes){
			_asd += `
					_data.${oMode.name}.${state.name} = tmp;`;
		}
	}
	_asd += `
				}`;
}
_asd += `
				state = 0;
			}`;
if (paramIfs){
	paramIfs += `
		else
		{
			${_asd}
		}`;
}
/*if (modeResolving){
	modeResolving += `
		else return collectError(p_error, "unsupported mode");`;
}*/
console.log(src.modes);

var _fields = {
	getters: '',
	valueHolders: '',
	help: `--config
	set alternative config file path (default is "~/${src.configFileName}")
--config-add
	copy current config and add options to it
--config-rewrite
	create new config and add options to it`,
	structs: '',
};
var _getters = (function(p_src, p_fields){
	var
		stack = [p_src],
		parent,
		child,
		alreadyUsed = new Set()
	;
	function resolveType(p){
		if (p === 'integer')
			return 'int';
		else if (p === 'string')
			return 'std::string';
		else if (p === 'boolean')
			return 'bool';
		return '';
	}
	function resolveValue(p){
		if (p.type === 'integer')
			return `${p.defaultValue}`;
		else if (p.type === 'string')
			return `"${p.defaultValue}"`;
		else if (p.type === 'boolean')
			return p.defaultValue?'true':'false';
		return '';
	}

	while (stack.length){
		parent = stack.shift();
		if (!parent.hasOwnProperty('helpIndent'))
			parent.helpIndent = '';
		if (parent.modes && parent.modes.length){
			for (child of parent.modes){
				stack.push(child);
				child.helpIndent = parent.helpIndent + '\t';
			}
		}
		if (parent.name){
			p_fields.help += `
--${parent.name}
	${parent.help}`;
			p_fields.structs += `
	struct ${parent.name[0].toUpperCase() + parent.name.slice(1)} : Common
	{`;
			p_fields.valueHolders += `
		${parent.name[0].toUpperCase() + parent.name.slice(1)} ${parent.name};`;
			p_fields.getters += `
	const ${parent.name[0].toUpperCase() + parent.name.slice(1)} & ${parent.name}() const {return _data.${parent.name};}`;
		}
		else{ // it's the root node
			p_fields.structs += `
	struct Common
	{`;
			p_fields.getters += `
	Mode mode() const {return _mode;}`;

		}
		if (parent.options && parent.options.length){
			if (parent.name){
				p_fields.help += `
	Options:`;
			}
			for (const oOption of parent.options){
				p_fields.structs += `
		${resolveType(oOption.type)} ${oOption.name} {${resolveValue(oOption)}};`;
				p_fields.help += `
${parent.helpIndent}--${oOption.name}
${parent.helpIndent}	${oOption.help} (default: ${resolveValue(oOption)})`;
			}
		}
		if (parent.name){
			p_fields.structs += `
	};`;
		}
		else{
			p_fields.structs += `
	};`;
		}
	}
	p_fields.valueHolders = `
	struct
	{${p_fields.valueHolders}
	} _data;
	Mode _mode {Mode::Undefined};`;
})(src, _fields);



var jsonParsingCode = '// JSON parsing: not implemented yet';




var toString = `retVal += "mode: ";`;
state = 0;
for (const oMode of src.modes){
	if (!state++) toString += `
	`;
	else toString += `
	else `;
	toString += `if (_mode == Mode::${oMode.inEnumName})
		retVal += "${oMode.name}";`;
}
toString += `
	else
		retVal += "<not set>";
	retVal += "\\nconfig file: \\"";
	retVal += _configPath;
	retVal += "\\"";
	if (_mode == Mode::Undefined)
		return retVal;`;
state = 0;
for (const oMode of src.modes){
	toString += state++ ? `
	else `: `
	`;
	toString += `if (_mode == Mode::${oMode.inEnumName})
	{`;
	let stateOpt = 0;
	const fPrintParam = function(p_opt, p_mode){
		var rv = '';
		rv += `
		retVal += "\\n${p_opt.name}: ";`;
		if (p_opt.type === 'string'){
			rv += `
		retVal += "\\"";
		retVal += _data.${p_mode.name}.${p_opt.name};
		retVal += "\\"";`;
		}
		else if (p_opt.type === 'integer'){
			rv += `;
		retVal += std::to_string(_data.${p_mode.name}.${p_opt.name});`
		}
		else if (p_opt.type === 'boolean'){
			rv += `;
		retVal += _data.${p_mode.name}.${p_opt.name} ? "true" : "false";`
		}
		return rv;
	};
	for (const oOpt of src.options){
		if (!stateOpt++){
			toString += `
		retVal += "\\n\\nCOMMON OPTIONS:\\n";`;
		}
		toString += fPrintParam(oOpt, oMode);
	}
	stateOpt = 0;
	for (const oOpt of oMode.options){
		if (!stateOpt++){
			toString += `
		retVal += "\\n\\nMODE SPECIFIC OPTIONS:\\n";`;
		}
		toString += fPrintParam(oOpt, oMode);
	}
	toString += `
	}`;
}



var result = {
	h : `#pragma once

#include <string>

class ${CLASSNAME}
{
public:
	enum class Mode
	{${_modes}
	};${_fields.structs}
	${CLASSNAME}() = default;
	int parse(int p_argc, char **p_argv, std::string *p_error); // return value: 0 if normal player start needed; 1 - if normal exit required; -1 - if error exit required (writing error description into p_error)
${_fields.getters}

	std::string toString() const;

private:
	const char *_configPath {nullptr};
${_fields.valueHolders}
};`,
	cpp: `#include "${CLASSFILENAME}.h"
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h> // exit(), atoi()
#include <fcntl.h>
#include <unistd.h>

inline int collectError(std::string *p_error, const std::string &p_message)
{
	if (p_error)
	{
		if (p_error->size() > 0)
			*p_error += ":\\n";
		*p_error += p_message;
	}
	return -1;
}

int ${CLASSNAME}::parse(int p_argc, char **p_argv, std::string *p_error)
{
	/* boris here:
	1. проходим на предмет запроса справки: тогда показываем справку и выходим
		попутно записываем значение --config - отсюда мы получаем итоговый (_)configPath.
	2. Вычитываем конфигурационный файл в копию структуры
	3. Проходим по аргументам: определяем режим работы
	4. Если режим работы совпадает с режимом в копии структуры, полученной от парсинга файла конфига, то копируем себе приватные поля копии структуры.
	5. Проходим по аргументам: правим отдельные поля приватных структур.

	проходим по нашему дереву:
		считываем из JSON общие параметры, режим и параметры, специфичные для режима.
	
	проходим по аргументам:
		считываем общие параметры, режим и параметры, специфичные для режима. Выводим сообщение об ошибке, если:
			* режим указан несколько раз
			* режим не указан, но указаны параметры, которых нет в числе общих параметров
			* режим указан, но не все указанные параметры есть в числе (общих и специфичных для указанного режима) параметров
	*/

	_configPath = nullptr;
	int state;
	/*
	states:
		0 - normal
		1 - config path expected
	*/
	state = 0;
	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		if (!strcmp(arg, "--help"))
		{
			std::cout << R"(${_fields.help})" << std::endl;
			exit(0);
		}
		else if (!strcmp(arg, "--config"))
		{
			state = 1;
		}
		else if (state)
		{
			if (arg[0] == '-')
			{
				return collectError(p_error, "--config: parameter value not present");
			}
			if (_configPath)
				return collectError(p_error, "only one time config file path can be set");
			_configPath = arg;
			state = 0;
		}
	}
	if (state)
		return collectError(p_error, "--config: parameter value not present");

	if (!_configPath)
		_configPath = "~/${src.configFileName}";

	if (int fd = open(_configPath, O_RDONLY))
	{
		${jsonParsingCode}
	}

	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		Mode cand = Mode::Undefined;
		${modeResolving}
		if (cand != Mode::Undefined)
		{
			if (_mode != Mode::Undefined){
				return collectError(p_error, "only one time mode can be set");
			};
			_mode = cand;
		}
	}
	if (_mode == Mode::Undefined)
	{
		return collectError(p_error, "mode not set");
	}
	state = 0;
	/*
	states:
		0 - normal
		1 - mode expected
	*/
	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		${paramIfs}
	}

	return 0;
}

std::string ${CLASSNAME}::toString() const
{
	std::string retVal;${toString}
	return retVal;
}
`
};
fs.writeFileSync(path.resolve(TARGET_DIR, `${CLASSFILENAME}.h`), result.h, 'utf8');
fs.writeFileSync(path.resolve(TARGET_DIR, `${CLASSFILENAME}.cpp`), result.cpp, 'utf8');
//console.log(result.h);
