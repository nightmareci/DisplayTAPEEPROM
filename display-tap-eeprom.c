/*

Originally made by Brandon McGriff. ( nightmareci@gmail.com )

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PLAYER1 0
#define PLAYER2 1

struct Record {
	char name[4];
	union {
		struct {
			unsigned grade : 5;
			bool greenline : 1;
			bool orangeline : 1;
			union {
				unsigned time : 20;
				unsigned score : 20;
			};
		};
		uint16_t levels[2];
	};
};

enum Medal {
	MEDAL_NONE,
	MEDAL_BRONZE,
	MEDAL_SILVER,
	MEDAL_GOLD
};

struct AwardedMedals {
	enum Medal CO : 2;
	enum Medal RO : 2;
	enum Medal RE : 2;
	enum Medal SK : 2;
	enum Medal ST : 2;
	enum Medal AC : 2;
};

#define BIG_ENDIAN_UINT16(bytes) (  \
	((uint16_t)(bytes)[0]<<8) | \
	((uint16_t)(bytes)[1]<<0)   \
)

#define BIG_ENDIAN_UINT32(bytes) (   \
	((uint32_t)(bytes)[0]<<24) | \
	((uint32_t)(bytes)[1]<<16) | \
	((uint32_t)(bytes)[2]<< 8) | \
	((uint32_t)(bytes)[3]<< 0)   \
)

// The formula used to compute the time (minutes, seconds, centiseconds) is the same as used in TAP's code.
// TAP's original hardware runs at a slightly higher frame rate (61.68Hz) than the developer-assumed 60Hz timing used here,
// so times shown in-game or shown by this program won't line up with wall-clock time of games on original hardware and accurate copies.
void FramesToTime(unsigned frames, unsigned *minutes, unsigned *seconds, unsigned *centiseconds) {
	*minutes = frames / 3600u;
	frames -= *minutes * 3600u;
	*seconds = frames / 60u;
	frames -= *seconds * 60u;
	*centiseconds = (frames * 100u) / 60u;
}

int main(int argc, char **argv) {
	uint8_t eeprom[0x100];
	const char *eepromFilename;
	const char *defaultFile;
	if (argc == 1) {
		eepromFilename = "eeprom";
		defaultFile = "default ";
	}
	else {
		eepromFilename = argv[1];
		defaultFile = "";
	}

	{
		FILE *eepromFile = fopen(eepromFilename, "rb");
		if (eepromFile == NULL) {
			fprintf(stderr, "ERROR: Failed opening %sTAP EEPROM file \"%s\".\n", defaultFile, eepromFilename);
			return EXIT_FAILURE;
		}
		fread(eeprom, sizeof(eeprom), 1u, eepromFile);
		fclose(eepromFile);
	}

	char **nameSubstitutions = NULL;
	int numSubstitutions = 0;
	if (argc > 2) {
		numSubstitutions = argc - 2;
		nameSubstitutions = malloc(sizeof(char*) * (argc - 2) * 2);
		for (int i = 2; i < argc; i++) {
			size_t nameToReplaceLen;
			for (nameToReplaceLen = 0u; nameToReplaceLen < strlen(argv[i]) && argv[i][nameToReplaceLen] != ':'; nameToReplaceLen++);
			if (nameToReplaceLen == 0u) {
				fprintf(stderr, "ERROR: Substitution spec \"%s\" contains no record name to substitute before the colon.\n", argv[i]);
				return EXIT_FAILURE;
			}
			size_t replacementNameLen = strlen(argv[i]) - 1 - nameToReplaceLen;
			if (replacementNameLen == 0u) {
				fprintf(stderr, "ERROR: Substitution spec \"%s\" contains no name to use as a substitute after the colon.\n", argv[i]);
				return EXIT_FAILURE;
			}

			nameSubstitutions[(i-2)*2 + 0] = malloc(nameToReplaceLen + 1u);
			nameSubstitutions[(i-2)*2 + 0][nameToReplaceLen] = '\0';
			strncpy(nameSubstitutions[(i-2)*2 + 0], argv[i], nameToReplaceLen);

			nameSubstitutions[(i-2)*2 + 1] = malloc(replacementNameLen + 1u);
			strcpy(nameSubstitutions[(i-2)*2 + 1], &argv[i][nameToReplaceLen + 1]);
		}
	}

	struct Record normal[3];
	struct Record masterSectionTimes[10];
	struct Record master[3];
	struct Record doubles[3];
	struct Record doublesCompletionLevels[3];
	struct AwardedMedals masterMedals[3];

	// Read records.

	for (int place = 0; place < 3; place++) {
		memcpy(normal[place].name, &eeprom[0xAC + place * 8], 4);
		normal[place].name[3] = '\0';

		normal[place].score = BIG_ENDIAN_UINT32(&eeprom[0xAC + place * 8 + 4]) & 0xFFFFF;

		memcpy(master[place].name, &eeprom[0x94 + place * 8], 4);
		master[place].name[3] = '\0';

		uint32_t masterData = BIG_ENDIAN_UINT32(&eeprom[0x94 + place * 8 + 4]);
		master[place].grade = masterData >> 27;
		master[place].greenline = (masterData >> 26) & 0x1;
		master[place].orangeline = (masterData >> 25) & 0x1;
		master[place].time = masterData & 0xFFFFF;

		uint16_t masterMedalData = BIG_ENDIAN_UINT16(&eeprom[0xF4 + place * 2]);
		masterMedals[place].AC = (masterMedalData >>  0) & 0x3;
		masterMedals[place].ST = (masterMedalData >>  2) & 0x3;
		masterMedals[place].SK = (masterMedalData >>  4) & 0x3;
		masterMedals[place].RE = (masterMedalData >>  6) & 0x3;
		masterMedals[place].RO = (masterMedalData >>  8) & 0x3;
		masterMedals[place].CO = (masterMedalData >> 10) & 0x3;

		// Player 1's name.
		memcpy(doubles[place].name, &eeprom[0xC4 + place * 8], 4);
		doubles[place].name[3] = '\0';
		// Player 2's name.
		memcpy(doublesCompletionLevels[place].name, &eeprom[0xDC + place * 8], 4);
		doublesCompletionLevels[place].name[3] = '\0';

		uint32_t doublesData = BIG_ENDIAN_UINT32(&eeprom[0xC4 + place * 8 + 4]);
		// Only player 1's grade is stored.
		doubles[place].grade = doublesData >> 27;
		doubles[place].time = doublesData & 0xFFFFF;
		// Can't underline in simple C programs, but seeing a completion level of 300 is enough indication.
		doubles[place].greenline = (doublesData >> 26) & 0x1;
		// Flag for orangeline is stored, but the flag is never set in the game state upon game completion.
		doubles[place].orangeline = (doublesData >> 25) & 0x1;
		uint32_t doublesCompletionLevelsData = BIG_ENDIAN_UINT32(&eeprom[0xDC + place * 8 + 4]);
		doublesCompletionLevels[place].levels[PLAYER1] = (doublesCompletionLevelsData >> 16) & 0xFFFF;
		doublesCompletionLevels[place].levels[PLAYER2] = doublesCompletionLevelsData & 0xFFFF;
	}

	for (int section = 0; section < 10; section++) {
		memcpy(masterSectionTimes[section].name, &eeprom[0x44 + section * 8], 4);
		masterSectionTimes[section].name[3] = '\0';
		uint32_t masterSectionTimeData = BIG_ENDIAN_UINT32(&eeprom[0x44 + section * 8 + 4]);
		masterSectionTimes[section].grade = masterSectionTimeData >> 27;
		masterSectionTimes[section].greenline = (masterSectionTimeData >> 26) & 0x1;
		masterSectionTimes[section].orangeline = (masterSectionTimeData >> 25) & 0x1;
		masterSectionTimes[section].time = masterSectionTimeData & 0xFFFFF;
	}

	// Read extra data (play status, etc.).

	uint32_t
		coinCount = BIG_ENDIAN_UINT32(&eeprom[0x2C]),
		demoWaitTime = BIG_ENDIAN_UINT32(&eeprom[0x30]),
		gameTime = BIG_ENDIAN_UINT32(&eeprom[0x34]);

	uint16_t
		playCount = BIG_ENDIAN_UINT16(&eeprom[0x38]),
		twinCount = BIG_ENDIAN_UINT16(&eeprom[0x3A]),
		versusCount = BIG_ENDIAN_UINT16(&eeprom[0x3C]);

	uint16_t initSeed = BIG_ENDIAN_UINT16(&eeprom[0x3E]);

	uint16_t
		playStatusChecksum = BIG_ENDIAN_UINT16(&eeprom[0x40]),
		rankingsChecksum = BIG_ENDIAN_UINT16(&eeprom[0xFA]),
		programChecksum = BIG_ENDIAN_UINT16(&eeprom[0xFC]);


	// Print out records.

	printf("[TAP] Normal:\n");
	const char *normalNameDashes = "----------------------------";
	for (int place = 0; place < 3; place++) {
		const char *name = normal[place].name;
		for (size_t i = 0u; i < numSubstitutions; i++) {
			if (strcmp(name, nameSubstitutions[i*2 + 0]) == 0) {
				name = nameSubstitutions[i*2 + 1];
				break;
			}
		}
		printf("% 4d--%s%s%06u pts @ -:--:--\n",
			place + 1,
			name,
			&normalNameDashes[strlen(name)],
			normal[place].score
		);
	}
	putchar('\n');

	const char *gradeNames[20] = {
		"9",
		"8",
		"7",
		"6",
		"5",
		"4",
		"3",
		"2",
		"1",
		"S1",
		"S2",
		"S3",
		"S4",
		"S5",
		"S6",
		"S7",
		"S8",
		"S9",
		"M",
		"Gm"
	};

	printf("[TAP] Master:\n");
	const char *gradeNameDashes = "---";
	const char *masterNameDashes = "----------------------";
	for (int place = 0; place < 3; place++) {
		const char *name = master[place].name;
		for (size_t i = 0u; i < numSubstitutions; i++) {
			if (strcmp(name, nameSubstitutions[i*2 + 0]) == 0) {
				name = nameSubstitutions[i*2 + 1];
			}
		}
		unsigned minutes, seconds, centiseconds;
		FramesToTime(master[place].time, &minutes, &seconds, &centiseconds);
		printf("--%d--%s%s%s%s - --- @ %02u:%02u:%02u - --/--/-- - ***************%s",
			place + 1,
			name,
			&masterNameDashes[strlen(name)],
			&gradeNameDashes[strlen(gradeNames[master[place].grade])],
			gradeNames[master[place].grade],
			minutes,
			seconds,
			centiseconds,
			master[place].orangeline ? " - Orangeline" : master[place].greenline ? " - Greenline" : ""
		);
		if (masterMedals[place].AC || masterMedals[place].ST || masterMedals[place].SK || masterMedals[place].RE || masterMedals[place].RO || masterMedals[place].CO) {
			printf("%sMedals: ", master[place].orangeline || master[place].greenline ? "; " : " - ");
			const char *awardedMedalNames[4] = { NULL, "bronze", "silver", "gold" };
			int numMedalsAwarded = 0;
			#define PRINT_MEDAL(name) \
			if (masterMedals[place].name != MEDAL_NONE) do { \
				printf("%s" #name " %s", numMedalsAwarded > 0 ? ", " : "", awardedMedalNames[masterMedals[place].name]); \
				numMedalsAwarded++; \
			} while (false)
			PRINT_MEDAL(AC);
			PRINT_MEDAL(ST);
			PRINT_MEDAL(SK);
			PRINT_MEDAL(RE);
			PRINT_MEDAL(RO);
			PRINT_MEDAL(CO);
			#undef PRINT_MEDAL
		}
		putchar('\n');
	}
	putchar('\n');

	printf("[TAP] Master Section Times:\n");
	for (int section = 0; section < 10; section++) {
		const char *name = masterSectionTimes[section].name;
		for (size_t i = 0u; i < numSubstitutions; i++) {
			if (strcmp(name, nameSubstitutions[i*2 + 0]) == 0) {
				name = nameSubstitutions[i*2 + 1];
			}
		}
		unsigned minutes, seconds, centiseconds;
		FramesToTime(masterSectionTimes[section].time, &minutes, &seconds, &centiseconds);
		printf("%03d - %03d--%s%s%s%s @ %02u:%02u:%02u - --/--/-- - ***************%s\n",
			section * 100,
			(section + 1) * 100 - 1,
			name,
			&masterNameDashes[strlen(name)],
			&gradeNameDashes[strlen(gradeNames[masterSectionTimes[section].grade])],
			gradeNames[masterSectionTimes[section].grade],
			minutes,
			seconds,
			centiseconds,
			masterSectionTimes[section].orangeline ? " - Orangeline" : masterSectionTimes[section].greenline ? " - Greenline" : ""
		);
	}
	putchar('\n');

	printf("[TAP] Doubles:\n");
	const char *doublesNameDashes = "-----------------------";
	for (int place = 0; place < 3; place++) {
		const char *names[2] = {doubles[place].name, doublesCompletionLevels[place].name};
		for (size_t i = 0u; i < numSubstitutions; i++) {
			for (int player = PLAYER1; player <= PLAYER2; player++) {
				if (strcmp(names[player], nameSubstitutions[i*2 + 0]) == 0) {
					names[player] = nameSubstitutions[i*2 + 1];
				}
			}
		}
		unsigned minutes, seconds, centiseconds;
		FramesToTime(doubles[place].time, &minutes, &seconds, &centiseconds);
		printf("%s%s %03u @ %02u:%02u:%02u @ %03u %s%s - %s (player 1) earned a grade of %s\n",
			names[PLAYER1],
			&doublesNameDashes[strlen(names[PLAYER1])],
			doublesCompletionLevels[place].levels[PLAYER1], 
			minutes,
			seconds,
			centiseconds,
			doublesCompletionLevels[place].levels[PLAYER2],
			&doublesNameDashes[strlen(names[PLAYER2])],
			names[PLAYER2],
			names[PLAYER1],
			gradeNames[doubles[place].grade]
		);
	}

	// Print out extra data (play status, etc.).

	unsigned minutes, seconds, centiseconds;
	printf("\n[TAP] Play Status:\n");
	printf("Coin Count: %"PRIu32"\n", coinCount);
	FramesToTime(demoWaitTime, &minutes, &seconds, &centiseconds);
	printf("Demo Wait Time: %02u:%02u:%02u\n", minutes, seconds, centiseconds);
	FramesToTime(gameTime, &minutes, &seconds, &centiseconds);
	printf("Game Time: %02u:%02u:%02u\n", minutes, seconds, centiseconds);
	printf("Play Count: %"PRIu16"\n", playCount);
	printf("Twin Count: %"PRIu16"\n", twinCount);
	printf("Doubles Count: %"PRIu16"\n", versusCount);

	printf("\n[TAP] Seed And Checksums:\n");
	printf("Init Seed: 0x%04"PRIX16"\n", initSeed);
	printf("Play Status Checksum: 0x%04"PRIX16"\n", playStatusChecksum);
	printf("Rankings Checksum: 0x%04"PRIX16"\n", rankingsChecksum);
	printf("Program Checksum: 0x%04"PRIX16"\n", programChecksum);

	for (size_t i = 0u; i < numSubstitutions; i++) {
		free(nameSubstitutions[(i*2) + 0]);
		free(nameSubstitutions[(i*2) + 1]);
	}
	free(nameSubstitutions);

	return EXIT_SUCCESS;
}
