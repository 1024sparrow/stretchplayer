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
	path = require('path'),
	configJsonParser = require('./configJsonParser.js')
;

var _modes = `
		Undefined = 0,`;
var modeResolving = '';
var paramIfs = `if (!strcmp(arg, "--config"))
			state = -1;
		else if (!strcmp(arg, "--config-gen"));`;
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
					std::string tmp = resolveEnvVarsAndTilda(arg);`;
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

//{{
var
	jsonSaveResulInteger = '',
	jsonSaveResulBoolean = '',
	jsonSaveResultString = '',
	jsonSaveMode = ''
;
var jsonParamKeys = (function(p_src){
	function fGetState(p2_strType){
		if (p2_strType === 'string')
			return 'InparamsValueStringStarting';
		if (p2_strType === 'boolean')
			return 'InparamsValueBooleanStarting';
		if (p2_strType === 'integer')
			return 'InparamsValueIntegerStarting';
		return '<<UNKNOWN_STATE_FOR_SUCH_FIELD_TYPE>>';
	}
	var jsonParamKeys = '';
	for (const oCommonParams of p_src.options){
		jsonParamKeys += jsonParamKeys ? `
			else ` : `
			`;
		//jsonParamKeys += `if (_state.key == "${oCommonParams.name}")
		//		_state.s = State::S::${fGetState(oCommonParams.type)};`;
		jsonParamKeys += `if (_state.key == "${oCommonParams.name}")
			{`;
		if (oCommonParams.type === 'string'){
			jsonParamKeys += `
				_state.s = State::S::InparamsValueStringStarting;`;
			jsonSaveResultString += jsonSaveResultString ? `
			else ` : `
			`;
			jsonSaveResultString += `if (_state.key == "${oCommonParams.name}")
			{`;
			for (const oMode of src.modes){
				jsonSaveResultString += `
				_conf->_data.${oMode.name}.${oCommonParams.name} = resolveEnvVarsAndTilda(_state.value);`;
			}
			jsonSaveResultString += `
			}`;
		}
		else if (oCommonParams.type === 'boolean'){
			jsonParamKeys += `
				_state.s = State::S::InparamsValueBooleanStarting;`;
			jsonSaveResulBoolean += jsonSaveResulBoolean ? `
			else ` : `
			`;
			jsonSaveResulBoolean += `if (_state.key == "${oCommonParams.name}")
			{`;
			for (const oMode of src.modes){
				jsonSaveResulBoolean += `
				_conf->_data.${oMode.name}.${oCommonParams.name} = newVal;`;
			}
			jsonSaveResulBoolean += `
			}`;
		}
		else if (oCommonParams.type === 'integer'){
			jsonParamKeys += `
				_state.intValue = {0, false};
				_state.s = State::S::InparamsValueIntegerStarting;`;
			jsonSaveResulInteger += jsonSaveResulInteger ? `
			else ` : `
			`;
			jsonSaveResulInteger += `if (_state.key == "${oCommonParams.name}")
			{`;
			for (const oMode of src.modes){
				jsonSaveResulInteger += `
				_conf->_data.${oMode.name}.${oCommonParams.name} = newVal;`;
			}
			jsonSaveResulInteger += `
			}`;
		}
			jsonParamKeys += `
			}`;
	}
	for (const oMode of src.modes){
		for (const oParam of oMode.options){
			jsonParamKeys += jsonParamKeys ? `
			else ` : `
			`;
			//jsonParamKeys += `if (_state.key == "${oCommonParams.name}")
			//		_state.s = State::S::${fGetState(oCommonParams.type)};`;
			jsonParamKeys += `if (_conf->_mode == Mode::${oMode.inEnumName} && _state.key == "${oParam.name}")
			{`;
			if (oParam.type === 'string'){
				jsonParamKeys += `
				_state.s = State::S::InparamsValueStringStarting;`;
				jsonSaveResultString += jsonSaveResultString ? `
			else ` : `
			`;
				jsonSaveResultString += `if (_conf->_mode == Mode::${oMode.inEnumName} && _state.key == "${oParam.name}")
			{
				_conf->_data.${oMode.name}.${oParam.name} = resolveEnvVarsAndTilda(_state.value);
			}`;
			}
			else if (oParam.type === 'boolean'){
				jsonParamKeys += `
				_state.s = State::S::InparamsValueBooleanStarting;`;
				jsonSaveResulBoolean += jsonSaveResulBoolean ? `
			else ` : `
			`;
				jsonSaveResulBoolean += `if (_conf->_mode == Mode::${oMode.inEnumName} && _state.key == "${oParam.name}")
			{
				_conf->_data.${oMode.name}.${oParam.name} = newVal;
			}`;
			}
			else if (oParam.type === 'integer'){
				jsonParamKeys += `
				_state.intValue = {0, false};
				_state.s = State::S::InparamsValueIntegerStarting;`;
				jsonSaveResulInteger += jsonSaveResulInteger ? `
			else ` : `
			`;
				jsonSaveResulInteger += `if (_conf->_mode == Mode::${oMode.inEnumName} && _state.key == "${oParam.name}")
			{
				_conf->_data.${oMode.name}.${oParam.name} = newVal;
			}`;
			}
			jsonParamKeys += `
			}`;
		}
		jsonSaveMode += jsonSaveMode ? `
			else ` : `
			`;
		jsonSaveMode += `if (_state.value == "${oMode.name}")
				_conf->_mode = Mode::${oMode.inEnumName};`;
	}
	return jsonParamKeys;
})(src);
//}}


var _fields = {
	getters: '',
	valueHolders: '',
	/*
--config-gen
	generate config-file content (print it to stdin) and normal exit`,
	*/
	help: `--config
	set alternative config file path (default is "~/${src.configFileName}")
	\${...} and ~ at the begin resolves to appropriate environment variable values
--config-gen
	generate config-file content (print it to stdin) and normal exit`,
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

var generateConfMode = '', generateConfParams = '';
for (const oMode of src.modes){
	generateConfMode += generateConfMode ? `
	else ` : ``;
	generateConfMode += `if (_mode == Mode::${oMode.inEnumName})
	{
		retVal += R"(
	"mode": "${oMode.name}",
	"parameters": {)";
	}`;

	generateConfParams += generateConfParams ? `
	else ` : ``;
	generateConfParams += `if (_mode == Mode::${oMode.inEnumName})
	{`;
	let opts = '', optSrc = [];
	for (const oParam of src.options){
		optSrc.push(oParam);
	}
	for (const oParam of oMode.options){
		optSrc.push(oParam);
	}
	for (const oParam of optSrc){
		opts += opts ? `
		retVal.push_back(',');` : ``;
		if (oParam.type === 'string'){
			opts += `
		retVal += R"(
		"${oParam.name}": ")";
		retVal += _data.${oMode.name}.${oParam.name};
		retVal += "\\"";`;
		}
		else if (oParam.type === 'integer'){
			opts += `
		retVal += R"(
		"${oParam.name}": )";
		retVal += std::to_string(_data.${oMode.name}.${oParam.name});`;
		}
		else if (oParam.type === 'boolean'){
			opts += `
		retVal += R"(
		"${oParam.name}": )";
		if (_data.${oMode.name}.${oParam.name})
			retVal += "true";
		else
			retVal += "false";`;
		}
	}
	generateConfParams += opts;
	generateConfParams += `
	}`;
}
generateConfMode += generateConfMode ? `
	else` : ``;
generateConfMode += `
	{
		retVal += R"(
	"parameters": {)";
	}`;
generateConfParams += `
	// else { ... } // set common only parameters. Not supported work without an active mode.`;



var result = {
	h : `// generated by generate_config.js
// source file is config.js

#pragma once

#include <string>

class ${CLASSNAME}
{
public:
	enum class Mode
	{${_modes}
	};${_fields.structs}
	${CLASSNAME}() = default;
	int parse(int p_argc, char **p_argv, const char *p_helpPrefix, const char *p_helpPostfix, std::string *p_error); // return value: 0 if normal player start needed; 1 - if normal exit required; -1 - if error exit required (writing error description into p_error)
${_fields.getters}

	std::string toString() const;

private:
	class JsonParser;
	int collectError(std::string *p_error, const std::string &p_message) const;
	static std::string resolveEnvVarsAndTilda(const std::string &p);
	std::string generateConf() const;

	std::string _configPath;
${_fields.valueHolders}
};`,


	cpp: `// generated by generate_config.js
// source file is config.js

#include "${CLASSFILENAME}.h"
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h> // exit(), atoi()
#include <fcntl.h>
#include <unistd.h>
#include <tuple>
#include <set>

class Configuration2::JsonParser
{
public:
	JsonParser(Configuration2 *p_conf);
	// p_force - error, if file not exists (or can not read)
	bool parse(const char *p_filepath, bool p_force, std::string *p_error);
private:
	enum class Error
	{
		NoError = 0,
		NotImplemented,
		UnhandledState,
		SystaxError,
		UnsupportedKeyUsed,
		InobjectCommaExpected,
		UnexpectedComma,
		TrueFalseExpected,
		DuplicatingKey,

		IncorrectMode,

		CanNotOpenFile,
		__Count
	};
	bool collectError(std::string *p_error, const std::string &p_message) const;
	void initParse();
	Error parseTick(char byte);

	bool isWhitespaceSymbol(char byte)
	{
		if (byte == ' ' || byte == '\\t' || byte == '\\r' || byte == '\\n')
			return true;
		return false;
	}
	bool isFilenameSymbol(char byte)
	{
		if (byte == ' ' || byte == '_')
			return true;
		if (byte >= ',' && byte <= '9')
			return true;
		if (byte >= 'A' && byte <= 'Z')
			return true;
		if (byte >= 'a' && byte <= 'z')
			return true;
		return false;
	}

	static const char * ERROR_CODE_DESCRIPTIONS[];
	static const char
		*USER_MARK,
		*TRUE,
		*FALSE
	;
	static const int
		USER_MARK_LEN,
		TRUE_LEN,
		FALSE_LEN
	;

	struct State
	{
		enum class S
		{
			Init,
			IntoGlobalObject,
			KeyStarting,
			KeyValueSeparator,
			ValueModeStarting,
			ValueMode,
			ValueParametersStarting,
			ValueParameters,
			ParametersKey,
			InparamsSeparator,
			InparamsValueStringStarting,
			InparamsValueString,
			InparamsValueBooleanStarting,
			InparamsValueBooleanTrue,
			InparamsValueBooleanFalse,
			InparamsValueIntegerStarting,
			InparamsValueInteger,
			InparamsValueFinished,
			ValueFinished,

			Finished
		} s {S::Init};
		static const char * str(S p)
		{
			static const std::tuple<S, const char *> srcData_123[] = {
				{ S::Init, "Init" },
				{ S::IntoGlobalObject, "IntoGlobalObject" },
				{ S::KeyStarting, "KeyStarting" },
				{ S::KeyValueSeparator, "KeyValueSeparator" },
				{ S::ValueModeStarting, "ValueModeStarting" },
				{ S::ValueMode, "ValueMode" },
				{ S::ValueParametersStarting, "ValueParametersStarting" },
				{ S::ValueParameters, "ValueParameters" },
				{ S::ParametersKey, "ParametersKey"},
				{ S::InparamsSeparator, "InparamsSeparator"},
				{ S::InparamsValueStringStarting, "InparamsValueStringStarting"},
				{ S::InparamsValueString, "InparamsValueString"},
				{ S::InparamsValueBooleanStarting, "InparamsValueBooleanStarting"},
				{ S::InparamsValueBooleanTrue, "InparamsValueBooleanTrue"},
				{ S::InparamsValueBooleanFalse, "InparamsValueBooleanFalse"},
				{ S::InparamsValueIntegerStarting, "InparamsValueIntegerStarting"},
				{ S::InparamsValueInteger, "InparamsValueInteger"},
				{ S::InparamsValueFinished, "InparamsValueFinished"},

				{ S::ValueFinished, "ValueFinished" },
				{ S::Finished, "Finished" }
			};
			for (auto o : srcData_123)
			{
				if (p == std::get<0>(o))
					return std::get<1>(o);
			}
			return "<incorrect>";
		}
		std::string key;
		std::string value;
		std::set<std::string> keysAlreadyUsed;
		std::set<std::string> inparamsKeysAlreadyUsed;
		struct
		{
			int value;
			bool negative;
		} intValue;
		int counter {0};
	} _state;
	Configuration2 *_conf;
};

int ${CLASSNAME}::parse(int p_argc, char **p_argv, const char *p_helpPrefix, const char *p_helpPostfix, std::string *p_error)
{
	/* Порядок разбора аргументов командной строки:

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

	_configPath.clear();
	bool needToGenerateConfig = false;
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
			if (p_helpPrefix)
			{
				std::cout << p_helpPrefix << std::endl;
			}
			std::cout << R"(${_fields.help})" << std::endl;
			if (p_helpPostfix)
			{
				std::cout << p_helpPostfix << std::endl;
			}
			exit(0);
		}
		else if (!strcmp(arg, "--config"))
		{
			state = 1;
		}
		else if (!strcmp(arg, "--config-gen"))
		{
			needToGenerateConfig = true;
		}
		else if (state)
		{
			if (arg[0] == '-')
			{
				return collectError(p_error, "--config: parameter value not present");
			}
			if (_configPath.size())
				return collectError(p_error, "only one time config file path can be set");
			_configPath = resolveEnvVarsAndTilda(arg);
			state = 0;
		}
	}
	if (state)
		return collectError(p_error, "--config: parameter value not present");

	{ // prevent argument duplication
		std::set<std::string> usedArguments;
		for (int iArg = 0 ; iArg < p_argc ; ++iArg)
		{
			const char *arg = p_argv[iArg];
			if (arg[0] == '-')
			{
				if (usedArguments.find(arg) != usedArguments.end())
					return collectError(p_error, std::string(arg) + ": duplicating argument");
				usedArguments.insert(arg);
			}
		}
	}

	state = 0;
	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		Mode cand = Mode::Undefined;
		${modeResolving}
		else
			continue;
		if (state == 0)
		{
			_mode = cand;
			state = 1;
		}
		else if (state == 1)
		{
			return collectError(p_error, "only one time mode can be set");
		}
	}

	bool usingDefaultConfig = _configPath.size();
	if (!usingDefaultConfig)
		_configPath = "~/${src.configFileName}";
	_configPath = resolveEnvVarsAndTilda(_configPath);

	Configuration2 confCopy = *this;
	JsonParser jsonParser(&confCopy);
	if (!jsonParser.parse(_configPath.c_str(), !usingDefaultConfig, p_error))
	{
		return collectError(p_error, "can not parse config");
	}
	if (_mode == Mode::Undefined && confCopy._mode != Mode::Undefined)
	{
		_mode = confCopy._mode;
	}
	if (confCopy._mode == Mode::Undefined || confCopy._mode == _mode)
	{
		_data = confCopy._data;
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

	if (needToGenerateConfig)
	{
		std::cout << generateConf() << std::endl;
		exit(0);
	}

	return 0;
}

std::string ${CLASSNAME}::toString() const
{
	std::string retVal;${toString}
	return retVal;
}

int Configuration2::collectError(std::string *p_error, const std::string &p_message) const
{
	if (p_error)
	{
		if (p_error->size() > 0)
			*p_error = ":\\n" + *p_error;
		*p_error = p_message + *p_error;
	}
	return -1;
}

/*
 * resolve "~/qwe/\${USER}/rty" to "/home/user/qwe/user/rty"
 * "\${...}" resolves to appropriate environment variable value
 * "~" resolves to home directory path only if it is placed in the begin
 */
std::string Configuration2::resolveEnvVarsAndTilda(const std::string &p)
{
	int state = 0, substate = 0;
	std::string envName, retVal;
	for (char ch : p)
	{
		if (state == 0)
		{
			if (ch == '~')
			{
				retVal += getenv("HOME");
				state = 1;
			}
			else if (ch == '$')
			{
				state = 2;
				substate = 0;
			}
			else
			{
				retVal.push_back(ch);
				state = 1;
			}
		}
		else if (state == 1)
		{
			if (ch == '$')
			{
				state = 2;
				substate = 0;
			}
			else
			{
				retVal.push_back(ch);
			}
		}
		else if (state == 2)
		{
			if (substate == 0)
			{
				if (ch == '{')
				{
					substate = 1;
					envName.clear();
				}
				else
				{
					retVal.push_back('$');
					retVal.push_back(ch);
				}
			}
			else if (substate == 1)
			{
				if (ch == '}')
				{
					retVal += getenv(envName.c_str());
					state = 1;
				}
				else
				{
					envName.push_back(ch);
				}
			}
		}
	}
	return retVal;
}

const char * Configuration2::JsonParser::ERROR_CODE_DESCRIPTIONS[] {
	"no error",
	"internal error (not implemented)",
	"internal error (unhandled state)",
	"syntax error",
	"unsupported key used",
	"comma between object key-value pairs expected",
	"unexpected comma not between objects in object",
	"true|false expected",
	"duplicating key",

	"there is not such mode",

	"can not open file",
	"" // for invalid error code
};

const char
	*Configuration2::JsonParser::USER_MARK = "{user}",
	*Configuration2::JsonParser::TRUE = "true",
	*Configuration2::JsonParser::FALSE = "false"
;
const int
	Configuration2::JsonParser::USER_MARK_LEN = strlen(USER_MARK),
	Configuration2::JsonParser::TRUE_LEN = strlen(TRUE),
	Configuration2::JsonParser::FALSE_LEN = strlen(FALSE)
;

Configuration2::JsonParser::JsonParser(Configuration2 *p_conf)
	: _conf(p_conf)
{
}

bool Configuration2::JsonParser::parse(const char *p_filepath, bool p_force, std::string *p_error)
{
	int fd = open(p_filepath, O_RDONLY);
	if (fd <= 0)
	{
		if (p_force)
		{
			return true;
		}
		return collectError(p_error, "can not open file");
	}
	const int bufferSize = 1024;
	char buffer[bufferSize];
	initParse();
	int lineNum = 1, colNum = 1;
	while(int result = read(fd, buffer, bufferSize))
	{
		if (result < 0)
		{
			close(fd);
			(void)collectError(p_error, strerror(errno));
			return collectError(p_error, "can not read file");
		}
		for (char i : buffer)
		{
			if (int err = static_cast<int>(parseTick(i))) // mode detecting
			{
				close(fd);
				sprintf(
					buffer,
					"line %i, column %i: %s.",
					lineNum,
					colNum,
					ERROR_CODE_DESCRIPTIONS[err]
				);
				return collectError(p_error, buffer);
			}
			if (_state.s == State::S::Finished)
				break;
			if (i == '\\n')
			{
				++lineNum;
				colNum = 1;
			}
			else
			{
				++colNum;
			}
		}
	}
	if (_state.s != State::S::Finished)
	{
		close(fd);
		return collectError(p_error, "file is incomplete");
	}

	lseek(fd, 0, SEEK_SET);
	lineNum = 1, colNum = 1;
	_state = State();
	while(int result = read(fd, buffer, bufferSize))
	{
		if (result < 0)
		{
			close(fd);
			(void)collectError(p_error, strerror(errno));
			return collectError(p_error, "can not read file");
		}
		for (char i : buffer)
		{
			if (int err = static_cast<int>(parseTick(i))) // parameters saving
			{
				close(fd);
				sprintf(
					buffer,
					"line %i, column %i: %s.",
					lineNum,
					colNum,
					ERROR_CODE_DESCRIPTIONS[err]
				);
				return collectError(p_error, buffer);
			}
			if (_state.s == State::S::Finished)
				break;
			if (i == '\\n')
			{
				++lineNum;
				colNum = 1;
			}
			else
			{
				++colNum;
			}
		}
	}
	close(fd);
	if (_state.s != State::S::Finished)
	{
		return collectError(p_error, "file is incomplete");
	}
	return true;
}

bool Configuration2::JsonParser::collectError(std::string *p_error, const std::string &p_message) const
{
	if (p_error)
	{
		if (p_error->size() > 0)
			*p_error = ":\\n" + *p_error;
		*p_error = p_message + *p_error;
	}
	return false;
}

void Configuration2::JsonParser::initParse()
{
	_state = State();
}

Configuration2::JsonParser::Error Configuration2::JsonParser::parseTick(char byte)
{
	//printf("%c\\t%s\\n", byte, State::str(_state.s));

	if (_state.s == State::S::Init)
	{
		if (isWhitespaceSymbol((byte)))
			;
		else if (byte == '{')
			_state.s = State::S::IntoGlobalObject;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::IntoGlobalObject)
	{
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == '"'){
			_state.s = State::S::KeyStarting;
			_state.key.clear();
		}
		else if (byte == '}'){
			_state.s = State::S::Finished;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::KeyStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::KeyValueSeparator;
		}
		else if (isFilenameSymbol(byte))
		{
			_state.key.push_back(byte);
		}
		else
		{
			return Error::SystaxError;
		}
	}
	else if (_state.s == State::S::KeyValueSeparator)
	{
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == ':')
		{
			if (_state.key == "mode")
				_state.s = State::S::ValueModeStarting;
			else if (_state.key == "parameters")
				_state.s = State::S::ValueParametersStarting;
			else
				return Error::UnsupportedKeyUsed;

			if (_state.keysAlreadyUsed.find(_state.key) == _state.keysAlreadyUsed.end())
				_state.keysAlreadyUsed.insert(_state.key);
			else
				return Error::DuplicatingKey;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueModeStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::ValueMode;
			_state.value.clear();
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueMode)
	{
		if (byte == '"')
		{
			_state.s = State::S::ValueFinished;
		}
		else if (isFilenameSymbol(byte))
		{
			_state.value.push_back(byte);
		}
		else
			return Error::SystaxError; // некорректный символ в строке имени режима
	}
	else if (_state.s == State::S::ValueParametersStarting)
	{
		if (byte == '{')
		{
			_state.s = State::S::ValueParameters;
			_state.key.clear();
			_state.value.clear();
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueParameters)
	{
		if (byte == '"')
		{
			_state.s = State::S::ParametersKey;
		}
		else if (byte == '}')
		{
			_state.s = State::S::IntoGlobalObject;
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ParametersKey)
	{
		if (byte == '"')
			_state.s = State::S::InparamsSeparator;
		else if (isFilenameSymbol(byte))
		{
			_state.key.push_back(byte);
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsSeparator)
	{
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == ':')
		{${jsonParamKeys}
			else
			{
				return Error::UnsupportedKeyUsed;
			}

			if (_state.inparamsKeysAlreadyUsed.find(_state.key) == _state.inparamsKeysAlreadyUsed.end())
				_state.inparamsKeysAlreadyUsed.insert(_state.key);
			else
				return Error::DuplicatingKey;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueStringStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::InparamsValueString;
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueString)
	{
		if (byte == '"')
		{${jsonSaveResultString}
			_state.s = State::S::InparamsValueFinished;
		}
		else if (isFilenameSymbol(byte))
			_state.value.push_back(byte);
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueBooleanStarting)
	{
		if (isWhitespaceSymbol(byte))
			;
		else  if (byte == TRUE[0])
		{
			_state.s = State::S::InparamsValueBooleanTrue;
		}
		else if (byte == FALSE[0])
		{
			_state.s = State::S::InparamsValueBooleanFalse;
		}
		else
			return Error::TrueFalseExpected;
		_state.counter = 1;
	}
	else if (_state.s == State::S::InparamsValueBooleanTrue)
	{
		if (TRUE[_state.counter] != byte)
			return Error::TrueFalseExpected;
		if (_state.counter == TRUE_LEN - 1)
		{
			bool newVal = true;${jsonSaveResulBoolean}
			_state.s = State::S::InparamsValueFinished;
		}
		++_state.counter;
	}
	else if (_state.s == State::S::InparamsValueBooleanFalse)
	{
		if (FALSE[_state.counter] != byte)
			return Error::TrueFalseExpected;
		if (_state.counter == FALSE_LEN - 1)
		{
			bool newVal = false;${jsonSaveResulBoolean}
			_state.s = State::S::InparamsValueFinished;
		}
		++_state.counter;
	}
	else if (_state.s == State::S::InparamsValueIntegerStarting)
	{
		if (byte == '-')
		{
			_state.intValue.negative = true;
			_state.s = State::S::InparamsValueInteger;
		}
		else if (byte >= '0' && byte <= '9')
		{
			_state.intValue.value = byte - '0';
			_state.s = State::S::InparamsValueInteger;
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueInteger)
	{
		if (byte >= '0' && byte <= '9')
			_state.intValue.value = _state.intValue.value * 10 + byte - '0';
		else if (byte == ',' || byte == '}' || isWhitespaceSymbol(byte))
		{
			int newVal = _state.intValue.value * (_state.intValue.negative ? -1 : 1);${jsonSaveResulInteger}
			_state.key.clear();
			_state.value.clear();
			if (byte == ',')
			{
				_state.s = State::S::ValueParameters;
			}
			if (byte == '}')
			{
				_state.s = State::S::Finished;
			}
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueFinished)
	{
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == ',')
		{
			_state.key.clear();
			_state.value.clear();
			_state.s = State::S::ValueParameters;
		}
		else if (byte == '}')
		{
			_state.key = "parameters";
			_state.s = State::S::ValueFinished;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueFinished)
	{
		if (_state.key == "mode")
		{${jsonSaveMode}
			else
				return Error::IncorrectMode;
		}
		else if (_state.key == "parameters")
			;
		else
		{
			puts("<EMERGENCY>"); // it's internal epic fail to be here (assert() this). Неподдерживаемые ключи должны были быть пресечены ранее (ещё до считывания значения)
			return Error::UnsupportedKeyUsed;
		}

		if (byte == ',')
			_state.s = State::S::IntoGlobalObject;
		else if (byte == '}')
			_state.s = State::S::Finished;
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::InobjectCommaExpected;
	}
	else
	{
		return Error::UnhandledState;
	}

	return Error::NoError;
}

std::string Configuration2::generateConf() const
{
	std::string retVal = "{";

	${generateConfMode}

	${generateConfParams}

	return retVal + R"(
	}
})";
}
`
};
fs.writeFileSync(path.resolve(TARGET_DIR, `${CLASSFILENAME}.h`), result.h, 'utf8');
fs.writeFileSync(path.resolve(TARGET_DIR, `${CLASSFILENAME}.cpp`), result.cpp, 'utf8');
//console.log(result.h);
