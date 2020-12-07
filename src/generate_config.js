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
for (let o of src.modes){
	o.inEnumName = o.name[0].toUpperCase() + o.name.slice(1);
	_modes += `
		${o.inEnumName},`;
}
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
	union
	{${p_fields.valueHolders}
	} _data;
	Mode _mode {Mode::Undefined};`;
})(src, _fields);



var result = {
	h : `#pragma once

#include <string>

class ${CLASSNAME}
{
public:
	enum class Mode
	{${_modes}
	};${_fields.structs}
	int parse(int p_argc, char **p_argv, std::string *p_error); // return value: 0 if normal player start needed; 1 - if normal exit required; -1 - if error exit required (writing error description into p_error)
${_fields.getters}

private:
	void printHelp();
${_fields.valueHolders}
};`,
	cpp: `#include "${CLASSFILENAME}.h"
#include <iostream>

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
}

void ${CLASSNAME}::printHelp()
{
	std::cout << R"(${_fields.help})" << std::endl;
}
`
};
fs.writeFileSync(path.resolve(TARGET_DIR, `${CLASSFILENAME}.h`), result.h, 'utf8');
fs.writeFileSync(path.resolve(TARGET_DIR, `${CLASSFILENAME}.cpp`), result.cpp, 'utf8');
//console.log(result.h);
