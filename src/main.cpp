/*
 * Copyright(c) 2009 by Gabriel M. Beddingfield <gabriel@teuton.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "config.h"

#include <stdio.h> // for printf, fgets
#include <stdlib.h> // atoll
#include <string.h>
#include <unistd.h> // read

#include "Configuration.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

#include "Engine.hpp"

int main(int argc, char* argv[])
{
    StretchPlayer::Configuration config(argc, argv);

    if(config.help() || ( !config.ok() )) {
	config.usage();
	if( !config.ok() ) return -1;
	return 0;
    }

    if( !config.quiet() ) {
	config.copyright();
    }

    std::unique_ptr<StretchPlayer::EngineMessageCallback> _engine_callback;
    std::unique_ptr<StretchPlayer::Engine> _engine(new StretchPlayer::Engine(&config));

    printf("enter a command (enter \"h\" for help).\n");
    char c;
    ssize_t dataLen;
    char str[1024];
    char *paramString;
    while (true)
    {
        dataLen = read(0, &str, 1024);
        if (dataLen <= 0)
        {
            printf("file IO error\n");
            break;
        }
        if (dataLen == 1)
            continue; // no command
        c = str[0];
        paramString = str + 1;
        str[dataLen - 1] = '\0';
        if (c == 'q')
            break;
        else if (c == 'h')
        {
            printf("########################################\n");
            printf("# Команды от пользователя:\n");
            printf("#  q - выйти.\n");
            printf("#  h - отобразить справку по консольным командам управления.\n");
            printf("#  1 - открыть. Сразу после этой команды необходимо ввести путь к файлу.\n");
            printf("#  2 - начать воспроизведение. Параметр: милисекунда начала\n");
            printf("#  3 - Начать воспроизведение. Парметры: милисекунды начала и конца воспроизведения.\n");
            printf("#  4 - Прекратить воспроизведение. Возвращает милисекунду останова.\n");
            printf("#  5 - запрос позиции воспроизведения. Возвращает милисекунду текущего воспроизведения.\n");
            printf("#  6 - задать скорость воспроизведения (в процентах).\n");
            printf("#  7 - задать смещение частот.\n");
            printf("#  8 - установить громкость (в процентах)");
            printf("#\n");
            printf("# Сообщения пользователю:\n");
            printf("#  0 - сообщение об ошибке (текст).\n");
            printf("#  1 - успешность открытия. Без параметров.\n");
            printf("#  4 - милисекунда останова.\n");
            printf("#  6 - скорость воспроизведения. Посылается в ответ на команды 2, 3 и 6.\n");
            printf("#  7 - смещение частот (число от -12 до 12). ПОсылается вответ на команды 2, 3 и 7.\n");
            // установить громкость
            //
            //
            //
            printf("########################################\n");
        }
        else if (c == '1')
        {
            _engine->load_song(paramString);
        }
        else if (c == '2')
        {
            long long ll = atoll(paramString);
            double d = ll/1000.;
            printf("%f\n", d);
            _engine->locate(d);
            _engine->play();
        }
        else if (c == '3')
        {
            long long ll1, ll2;
            const char *s = strtok(paramString, " ");
            if (s)
                ll1 = atoll(s);
            else
            {
                printf("error");
                continue;
            }
            s = strtok(NULL, " ");
            if (s)
                ll2 = atoll(s);
            else
            {
                printf("error");
                continue;
            }
            printf("%lli - %lli\n", ll1, ll2);
            double d1 = ll1/1000.;
            _engine->locate(d1);
            _engine->play();
        }
        else if (c == '4')
        {
            _engine->stop();
        }
        else if (c == '5')
        {
        }
        else if (c == '6')
        {
            short i = atoi(paramString);
            float d = i/100.;
            _engine->set_stretch(d);
        }
        else if (c == '7')
        {
            short i = atoi(paramString);
            _engine->set_pitch(i);
        }
        else if (c == '8')
        {
            short i = atoi(paramString);
            float d = i / 100.;
            _engine->set_volume(d);
        }
        else
        {
            printf("=============: %c\n", c);
            printf("else: \"%s\"\n", paramString);
        }
    }
    return 0;
}
