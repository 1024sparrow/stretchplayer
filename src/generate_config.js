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
var paramIfs = '';
var stateCounter = [];
for (let o of src.options){
	if (paramIfs){
		paramIfs += '\n\t\telse ';
	}
	else{
		paramIfs += '\n\t\t';
	}
	stateCounter.push({
		state: stateCounter.length + 1,
		type: o.type,
		name: o.name
	});
	paramIfs += `if (!strcmp(arg, "--${o.name}"))
			state = ${stateCounter.length};`;
}
var _inModeParams;
for (let o of src.modes){
	o.inEnumName = o.name[0].toUpperCase() + o.name.slice(1);
	_modes += `
		${o.inEnumName},`;
	if (modeResolving)
		modeResolving += '\n\t\telse ';
	else
		modeResolving += '\n\t\t';
	modeResolving += `if (!strcmp(arg, "--${o.name}")) _mode = Mode::${o.inEnumName};`;

	_inModeParams = '';
	for (const oInModeParams of o.options){
		stateCounter.push({
			state: stateCounter.length + 1,
			type: oInModeParams.type,
			target: `_data.${o.name}.${oInModeParams.name}`,
			name: oInModeParams.name
		});
		if (_inModeParams)
			_inModeParams += '\n\t\t\telse ';
		else
			_inModeParams += '\n\t\t\t';
		_inModeParams += `if (!strcmp(arg, "--${oInModeParams.name}"))
				state = ${stateCounter.length};`;
		/*if (oInModeParams.type === 'integer'){
			_inModeParams += `
				_data.${o.name}.${oInModeParams.name} = atoi(arg);`;
		}
		else if (oInModeParams.type === 'string'){
			_inModeParams += `
				_data.${o.name}.${oInModeParams.name} = arg;`;
		}
		else if (oInModeParams.type === 'boolean'){
			_inModeParams += `
				if (!strcmp(arg, "true"))
					_data.${o.name}.${oInModeParams.name} = true;
				else if (!strcmp(arg, "false"))
					_data.${o.name}.${oInModeParams.name} = false;
				else
					return collectError(p_error, "--${oInModeParams.name}: true|false expected");`
		}
		else{
			console.log('incorrect parameter type pointed');
			process.exit(1);
		}*/
		//_inModeParams += '\n\t\t\t}';
	}
	paramIfs += `
		else if (_mode == Mode::${o.inEnumName})
		{
			/*if (state)
				return collectError(p_error, "parameter value missing");*/
			${_inModeParams}`;
	paramIfs += `
		}`;
}
var _asd = `if (state == 0)
			{
				// свободный аргумент (вне параметров)
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
	help: '',
	structs: ''
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
	if (p_fields.help){
		p_fields.help = p_fields.help.slice(1); // remove starting new-line symbol
	}
	p_fields.valueHolders = `
	struct
	{${p_fields.valueHolders}
	} _data;
	Mode _mode {Mode::Undefined};`;
})(src, _fields);



var jsonParsingCode = '// JSON parsing: not implemented yet';



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

private:
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
	проходим на предмет запроса справки: тогда показываем справку и выходим
		попутно записываем значение --config - отсюда мы получаем итоговый (_)configPath.
	проходим по нашему дереву:
		считываем из JSON общие параметры, режим и параметры, специфичные для режима.
	
	проходим по аргументам:
		считываем общие параметры, режим и параметры, специфичные для режима. Выводим сообщение об ошибке, если:
			* режим указан несколько раз
			* режим не указан, но указаны параметры, которых нет в числе общих параметров
			* режим указан, но не все указанные параметры есть в числе (общих и специфичных для указанного режима) параметров
	*/

	const char *configPath = nullptr;
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
			if (configPath)
				return collectError(p_error, "only one time config file path can be set");
			configPath = arg;
		}
	}
	if (state)
		return collectError(p_error, "--config: parameter value not present");

	if (int fd = open(configPath, O_RDONLY))
	{
		${jsonParsingCode}
	}

	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		if (_mode != Mode::Undefined){
			return collectError(p_error, "only one time mode can be set");
		};
		${modeResolving}
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

/*void ${CLASSNAME}::printHelp()
{
}*/
`
};
fs.writeFileSync(path.resolve(TARGET_DIR, `${CLASSFILENAME}.h`), result.h, 'utf8');
fs.writeFileSync(path.resolve(TARGET_DIR, `${CLASSFILENAME}.cpp`), result.cpp, 'utf8');
//console.log(result.h);
