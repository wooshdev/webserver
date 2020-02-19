/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * This file contains some non-specific tools.
 */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* DO care about case */
#define CASEFLAG_DONT_IGNORE 0

/* faster for small lists because this doesn't allocate */
#define CASEFLAG_IGNORE_A 1

/* faster for big lists */
#define CASEFLAG_IGNORE_B 2

/**
 * Terminology
 *   strswitch: string switch
 *   in: input string
 *   list: array of strings to check
 *   size: the size of check
 *   ignore_case: (bool) should we ignore upper-/lowercase differences
 * 
 * Description:
 *   This function is useful in switch statements, because you
 *   can't simply switch over a string in C. Therefore, this 
 *   function returns the index of 'list' wherein in the value 
 *   of the item in the array at that position is the same as
 *   the supplied string, 'in'.
 * 
 * Returns:
 *   This function returns the index of the 'list' array wherein
 * 		the value is, or -1 if it wasn't found.
 * 
 * Notes:
 *   1. This function contains case-sensitive string compares.
 * 
 * Example:
 *		const char *clicked = "cancel";
 *		char *button_list[] = { "yes", "no", "cancel", "help" };
 *		switch (strswitch(clicked, button_list, sizeof(button_list))) {
 *			case 0:
 *				save();
 * 				break;
 *			case 1:
 *				discard();
 *				break;
 *			case 2:
 *        cancel();
 *				puts("cancelled exit");
 *				break;
 * 			case 3:
 * 				show_help_dialog();
 *				break;
 *			default:
 * 				puts("Illegal State");
 *				break;
 *		}
 */
int strswitch(const char *in, const char **list, size_t size, int case_flag);

/**
 * Description:
 *   This function check is the second string is the first part of the first string.
 *
 * Parameter:
 *   const char *
 *     The string to check.
 *   const char *
 *     The string to check with. This should be the string that is the first part of 
 *     the first string, or not.
 *
 * Return Value:
 *   (boolean) 1 means it does start with the string, 0 if it doesn't.
 *
 * Examples:
 *   strstartsw("hello", "h") => 1				(explanation: yes, "h" is the first part of "hello")
 *   strstartsw("hello", "hello, goodbye") => 0	(explanation: no, "hello, goodbye" is not the first part, of "hello". You should switch the parameters around.)
 *   strstartsw("hello", "HELLO") => 0			(explanation: this string function is case-sensitive)
 *   strstartsw("hello, "") => 1					(explanation: the second parameter can be empty.)
 *   strstartsw("hello", "hello") => 1			(explanation: this function can be used as strcmp!)
 *   
 */
int strstartsw(const char *, const char *);

/**
 * Copied from: https://stackoverflow.com/questions/252782/strdup-what-does-it-do-in-c#252802
 * Should conform to: https://linux.die.net/man/3/strdup
 */
char *strdup(const char *src);
#endif /* UTIL_H */
 
 
