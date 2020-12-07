/*
Исходные данные для генератора класса-отображения конфига.
Конфиг располагается в корне домашней директории или по пути, указанном опцией --config
У генерируемого класса можно запросить текст справки по опциям.
Значение каждой опции задаётся в следующем порядке:
  1. Значение по умолчанию
  2. Значение, указанное в файле конфига (если такой задан)
  3. Значение, указанное соответствующим аргументом командной строки
*/

/*
Вы можете в разных режимах использовать параметры с одинаковыми именами, НО
в таком случае вы обязаны обеспечить одинаковость их типов
*/
module.exports = {
	configFileName: '.stretchplayer.conf', // in home directory
	options:[ // common options. "--" at begin  passed.
		{
			name: 'sampleRate',
			type: 'integer',
			defaultValue: 44100,
			help: 'sample rate to use for ALSA'
		},
		{
			name: 'mono',
			type: 'boolean',
			defaultValue: false,
			help: 'merge all sound channels into the one: make it mono'
		},
		{
			name: 'mic',
			type: 'boolean',
			defaultValue: false,
			help: 'use microphone for sound catching'
		},
		{
			name: 'shift',
			type: 'integer',
			defaultValue: 0,
			help: 'right channel ahead of left (in seconds). Automaticaly make it mono.'
		},
		{
			name: 'stretch',
			type: 'integer',
			defaultValue: 100,
			help: 'playing speed (in percents)'
		},
		{
			name: 'pitch',
			type: 'integer',
			defaultValue: 0,
			help: 'frequency shift (number from -12 to 12)'
		}
	],
	modes:[
		{
			name: 'alsa',
			help: 'use ALSA for audio',
			options:[ // alsa-specific options
				{
					name: 'device',
					type: 'string',
					defaultValue: 'default',
					help: 'device to use for ALSA'
				},
				{
					name: 'periodSize',
					type: 'integer',
					defaultValue: 1024,
					help: 'period size to use for ALSA'
				},
				{
					name: 'periods',
					type: 'integer',
					defaultValue: 2,
					help: 'periods per buffer for ALSA'
				}
			]
		},
		{
			name: 'fake',
			help: 'use FIFO-s to write playback-data and read capture-data',
			options:[
				{
					name: 'fifoPlayback',
					type: 'string',
					defaultValue: '~/.stretchplayer-playback.fifo',
					help: 'filepath to the FIFO to write playback into'
				},
				{
					name: 'fifoCapture',
					type: 'string',
					defaultValue: '~/.stretchplayer-capture.fifo',
					help: 'filepath to the FIFO to read capture from'
				}
			]
		},
		{
			name: 'jack',
			help: 'use JACK for audio',
			options:[
				{
					name: 'noAutoconnect',
					type: 'boolean',
					defaultValue: false,
					help: 'disable auto-connection ot ouputs (for JACK)'
				}
			]
		}
	]
};
