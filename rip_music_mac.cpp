/*
       _________ __                 __
      /   _____//  |_____________ _/  |______     ____  __ __  ______
      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
             \/                  \/          \//_____/            \/
  ______________________                           ______________________
                        T H E   W A R   B E G I N S
         Stratagus - A free fantasy real time strategy game engine

    rip_music_unix.c - rip audio CD on Unix with cdparanoia
    Copyright (C) 2011  Pali Rohár <pali.rohar@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libgen.h>


#include "rip_music.h"

char * find_mnt_dir(const char * dir) {
	//TODO: implement
	return NULL;

}

char * find_dev(const char * mnt_dir) {
	//TODO: implement
	return NULL;

}

int spawnvp(const char * file, char * const argv[]) {

	int status;
	pid_t pid = fork();

	if ( pid < 0 ) {

		fprintf(stderr, "Error: Cannot fork: %s\n", strerror(errno));
		return -errno;

	}

	if ( pid == 0 ) {

		execvp(file, argv);
		fprintf(stderr, "Error: Cannot exec: %s\n", strerror(errno));
		_exit(errno);

	}

	if ( waitpid(pid, &status, 0) == pid )
		return WEXITSTATUS(status);
	else
		return -1;

}

int RipMusic(int expansion_cd, const char * data_dir, const char * dest_dir) {

	struct stat st;
	char cdparanoia[] = "cdparanoia";
	char * args[6] = { cdparanoia, "-d", "", "-v", "-Q", NULL };
	char * mnt_dir;
	char * dev;
	int count = 0;
	int i;

	if ( stat(data_dir, &st) != 0 ) {

		fprintf(stderr, "Error: Cannot stat %s: %s\n", data_dir, strerror(errno));
		return 1;

	}

	mnt_dir = find_mnt_dir(data_dir);

	if ( ! mnt_dir ) {

		fprintf(stderr, "Error: Cannot find mount dir\n");
		return 1;

	}

	dev = find_dev(mnt_dir);

	if ( ! dev ) {

		fprintf(stderr, "Error: Cannot find device for mount dir %s\n", mnt_dir);
		free(mnt_dir);
		return 1;

	}

	free(mnt_dir);

	printf("Found CD-ROM device: %s\n", dev);
	fflush(stdout);

	args[2] = dev;

	if ( spawnvp(cdparanoia, args) != 0 ) {

		free(dev);
		return 1;

	}

	for ( i = 0; MusicNames[i]; ++i ) {

		char num[3];
		char file[PATH_MAX];

		if ( ! expansion_cd && ! MusicNames[i+1] )
			break;

		snprintf(num, sizeof(num), "%d", i+2);
		snprintf(file, sizeof(file), "%s/%s.wav", dest_dir, MusicNames[i]);

		args[3] = num;
		args[4] = file;

		if ( spawnvp(cdparanoia, args) == 0 )
			++count;

	}

	free(dev);

	if ( count == 0 )
		return 1;
	else
		return 0;

}
