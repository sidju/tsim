
#ifndef TS_general_H_
#define TS_general_H_

#define NEW(o) ((o*)(malloc(sizeof(o))))

#define errputs(string) fprintf(stderr, "tsim: "string"\n")
#define errprintf(string, ...) fprintf(stderr, "tsim: "string"\n", __VA_ARGS__)

/* using float/double depending on 32/64-bit system */
#ifdef __LP64__
typedef double tFloat;
#else
typedef float tFloat;
#endif

#endif /* TS_general_H_ */
