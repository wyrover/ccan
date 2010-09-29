#ifndef CCAN_OPT_H
#define CCAN_OPT_H
#include <ccan/typesafe_cb/typesafe_cb.h>
#include <stdbool.h>

/* You can use this directly to build tables, but the macros will ensure
 * consistency and type safety. */
enum opt_flags {
	OPT_NOARG = 1,		/* -f/--foo */
	OPT_HASARG = 2,		/* -f arg/--foo=arg/--foo arg */
	OPT_SUBTABLE = 4,	/* Actually, longopt points to a subtable... */
	OPT_END = 8,		/* End of the table. */
};

struct opt_table {
	const char *longopt; /* --longopt, or NULL */
	char shortopt; /* -s, or 0 */
	enum opt_flags flags;
	char *(*cb)(void *arg); /* OPT_NOARG */
	char *(*cb_arg)(const char *optarg, void *arg); /* OPT_HASARG */
	void *arg;
	const char *desc;
};

/**
 * OPT_WITHOUT_ARG() - macro for initializing an opt_table entry (without arg)
 * @longopt: the name of the argument (eg. "foo" for "--foo"), or NULL.
 * @shortopt: the character of the argument (eg. 'f' for "-f"), or 0.
 * @cb: the callback when the option is found.
 * @arg: the argument to hand to @cb.
 *
 * This is a typesafe wrapper for intializing a struct opt_table.  The callback
 * of type "char *cb(type *)", "char *cb(const type *)" or "char *cb(void *)",
 * where "type" is the type of the @arg argument.
 *
 * At least one of @longopt and @shortopt must be non-zero.  If the
 * @cb returns non-NULL, opt_parse() will stop parsing, use the returned
 * string to form an error message, free() the string and return false.
 *
 * See Also:
 *	OPT_WITH_ARG()
 */
#define OPT_WITHOUT_ARG(longopt, shortopt, cb, arg)	\
	(longopt), (shortopt), OPT_CB_NOARG((cb), (arg))

/**
 * OPT_WITH_ARG() - macro for initializing long and short option (with arg)
 * @longopt: the name of the argument (eg. "foo" for "--foo <arg>"), or NULL.
 * @shortopt: the character of the argument (eg. 'f' for "-f <arg>"), or 0.
 * @cb: the callback when the option is found (along with <arg>).
 * @arg: the argument to hand to @cb.
 *
 * This is a typesafe wrapper for intializing a struct opt_table.  The callback
 * is of type "bool cb(const char *, type *)",
 * "bool cb(const char *, const type *)" or "bool cb(const char *, void *)",
 * where "type" is the type of the @arg argument.  The first argument to the
 * @cb is the argument found on the commandline.
 *
 * At least one of @longopt and @shortopt must be non-zero.  If the
 * @cb returns false, opt_parse() will stop parsing and return false.
 *
 * See Also:
 *	OPT_WITH_ARG()
 */
#define OPT_WITH_ARG(longopt, shortopt, cb, arg)	\
	(longopt), (shortopt), OPT_CB_ARG((cb), (arg))

/**
 * OPT_SUBTABLE() - macro for including another table inside a table.
 * @table: the table to include in this table.
 * @desc: description of this subtable (for opt_usage()) or NULL.
 *
 * The @desc field can be opt_table_hidden to hide the options from opt_usage().
 */
#define OPT_SUBTABLE(table, desc)					\
	{ (const char *)(table), sizeof(_check_is_entry(table)),	\
	  OPT_SUBTABLE,	NULL, NULL, NULL, (desc) }

/**
 * opt_table_hidden - string for undocumented option tables.
 *
 * This can be used as the desc option to OPT_SUBTABLE or passed to
 * opt_register_table() if you want the options not to be shown by
 * opt_usage().
 */
extern const char opt_table_hidden[];

/**
 * OPT_ENDTABLE - macro to create final entry in table.
 *
 * This must be the final element in the opt_table array.
 */
#define OPT_ENDTABLE { NULL, 0, OPT_END }

/**
 * opt_register_table - register a table of options
 * @table: the table of options
 * @desc: description of this subtable (for opt_usage()) or NULL.
 *
 * The table must be terminated by OPT_ENDTABLE.
 *
 * Example:
 * static struct opt_table opts[] = {
 * 	{ OPT_WITHOUT_ARG("verbose", 'v', opt_inc_intval, &verbose),
 * 	  "Verbose mode (can be specified more than once)" },
 * 	{ OPT_WITHOUT_ARG("usage", 0, opt_usage_and_exit,
 * 			  "args...\nA silly test program."),
 * 	  "Print this message." },
 * 	OPT_ENDTABLE
 * };
 *
 * ...
 *	opt_register_table(opts, NULL);
 */
void opt_register_table(const struct opt_table table[], const char *desc);

/**
 * opt_register_noarg - register an option with no arguments
 * @longopt: the name of the argument (eg. "foo" for "--foo"), or NULL.
 * @shortopt: the character of the argument (eg. 'f' for "-f"), or 0.
 * @cb: the callback when the option is found.
 * @arg: the argument to hand to @cb.
 * @desc: the verbose desction of the option (for opt_usage()), or NULL.
 *
 * This is used for registering a single commandline option which takes
 * no argument.
 *
 * The callback is of type "bool cb(type *)", "bool cb(const type *)"
 * or "bool cb(void *)", where "type" is the type of the @arg
 * argument.
 *
 * At least one of @longopt and @shortopt must be non-zero.  If the
 * @cb returns false, opt_parse() will stop parsing and return false.
 */
#define opt_register_noarg(longopt, shortopt, cb, arg, desc)	\
	_opt_register((longopt), (shortopt), OPT_CB_NOARG((cb), (arg)), (desc))

/**
 * opt_register_arg - register an option with an arguments
 * @longopt: the name of the argument (eg. "foo" for "--foo"), or NULL.
 * @shortopt: the character of the argument (eg. 'f' for "-f"), or 0.
 * @cb: the callback when the option is found.
 * @arg: the argument to hand to @cb.
 * @desc: the verbose desction of the option (for opt_usage()), or NULL.
 *
 * This is used for registering a single commandline option which takes
 * an argument.
 *
 * The callback is of type "bool cb(const char *, type *)",
 * "bool cb(const char *, const type *)" or "bool cb(const char *, void *)",
 * where "type" is the type of the @arg argument.  The first argument to the
 * @cb is the argument found on the commandline.
 *
 * At least one of @longopt and @shortopt must be non-zero.  If the
 * @cb returns false, opt_parse() will stop parsing and return false.
 *
 * Example:
 *	opt_register_arg("explode", 'e', explode_cb, NULL,
 *			 "Make the machine explode (developers only)");
 */
#define opt_register_arg(longopt, shortopt, cb, arg, desc)	\
	_opt_register((longopt), (shortopt), OPT_CB_ARG((cb), (arg)), (desc))

/**
 * opt_parse - parse arguments.
 * @argc: pointer to argc
 * @argv: argv array.
 * @errlog: the function to print errors (usually opt_log_stderr).
 *
 * This iterates through the command line and calls callbacks registered with
 * opt_register_table()/opt_register_arg()/opt_register_noarg().  If there
 * are unknown options, missing arguments or a callback returns false, then
 * an error message is printed and false is returned.
 *
 * On success, argc and argv are adjusted so only the non-option elements
 * remain, and true is returned.
 *
 * Example:
 *	if (!opt_parse(argc, argv, opt_log_stderr)) {
 *		printf("%s", opt_usage(argv[0], "<args>..."));
 *		exit(1);
 *	}
 */
bool opt_parse(int *argc, char *argv[], void (*errlog)(const char *fmt, ...));

/**
 * opt_log_stderr - print message to stderr.
 * @fmt: printf-style format.
 *
 * This is the standard helper for opt_parse, to print errors.
 */
void opt_log_stderr(const char *fmt, ...);

/**
 * opt_invalid_argument - helper to allocate an "Invalid argument '%s'" string
 * @arg: the argument which was invalid.
 *
 * This is a helper for callbacks to return a simple error string.
 */
char *opt_invalid_argument(const char *arg);

/**
 * opt_usage - create usage message
 * @argv0: the program name
 * @extra: extra details to print after the initial command, or NULL.
 *
 * Creates a usage message, with the program name, arguments, some extra details
 * and a table of all the options with their descriptions.
 *
 * The result should be passed to free().
 */
char *opt_usage(const char *argv0, const char *extra);

/* Standard helpers.  You can write your own: */
/* Sets the @b to true. */
char *opt_set_bool(bool *b);
/* Sets @b based on arg: (yes/no/true/false). */
char *opt_set_bool_arg(const char *arg, bool *b);
/* The inverse */
char *opt_set_invbool(bool *b);
char *opt_set_invbool_arg(const char *arg, bool *b);

/* Set a char *. */
char *opt_set_charp(const char *arg, char **p);

/* Set an integer value, various forms.  Sets to 1 on arg == NULL. */
char *opt_set_intval(const char *arg, int *i);
char *opt_set_uintval(const char *arg, unsigned int *ui);
char *opt_set_longval(const char *arg, long *l);
char *opt_set_ulongval(const char *arg, unsigned long *ul);

/* Increment. */
char *opt_inc_intval(int *i);

/* Display version string to stdout, exit(0). */
char *opt_show_version_and_exit(const char *version);

/* Display usage string to stdout, exit(0). */
char *opt_usage_and_exit(const char *extra);

/* Below here are private declarations. */
/* Resolves to the four parameters for non-arg callbacks. */
#define OPT_CB_NOARG(cb, arg)				\
	OPT_NOARG,					\
	cast_if_any(char *(*)(void *), (cb), &*(cb),	\
		    char *(*)(typeof(*(arg))*),		\
		    char *(*)(const typeof(*(arg))*),	\
		    char *(*)(const typeof(*(arg))*)),	\
	NULL, (arg)

/* Resolves to the four parameters for arg callbacks. */
#define OPT_CB_ARG(cb, arg)						\
	OPT_HASARG, NULL,						\
	cast_if_any(char *(*)(const char *,void *), (cb), &*(cb),	\
		    char *(*)(const char *, typeof(*(arg))*),		\
		    char *(*)(const char *, const typeof(*(arg))*),	\
		    char *(*)(const char *, const typeof(*(arg))*)),	\
	(arg)

/* Non-typesafe register function. */
void _opt_register(const char *longopt, char shortopt, enum opt_flags flags,
		   char *(*cb)(void *arg),
		   char *(*cb_arg)(const char *optarg, void *arg),
		   void *arg, const char *desc);

/* We use this to get typechecking for OPT_SUBTABLE */
static inline int _check_is_entry(struct opt_table *e) { return 0; }

#endif /* CCAN_OPT_H */