/*
    dblist.c - dumps the contents of a database table to stdout
    Copyright (C) 1989,1990  David A. Snyder
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "decimal.h"
#include "isam.h"
#include "sqltabs.h"
#include "sqltypes.h"

#define SUCCESS	0
#define SET(x) x=1
#define RESET(x) x=0

char	path[128], systab[SYSTABLES_SIZE];
char	*database = NULL, *index = NULL, *table = NULL;
char	*syscol = NULL, *sysind = NULL;
char	*record = NULL, *tmp_char = NULL, *value = NULL;
int	fd;
struct DBVIEW {
	short	vwstart;
	short	vwlen;
	short	vwprec;
	short	vwscale;
} *dbview;

main(argc, argv)
int	argc;
char	*argv[];
{

	extern char	*optarg;
	extern int	optind, opterr;
	char	filename[128], tmp_table[19];
	int	c, dflg = 0, errflg = 0, hflg = 0, iflg = 0, tflg = 0;
	struct dictinfo dict;
	void	exit();

	/* Print copyright message */
	(void)fprintf(stderr, "DBLIST version 1.0, Copyright (C) 1989,1990 David A. Snyder\n\n");

	/* get command line options */
	while ((c = getopt(argc, argv, "d:hi:t:")) != EOF)
		switch (c) {
		case 'd':
			dflg++;
			database = optarg;
			break;
		case 'h':
			hflg++;
			break;
		case 'i':
			iflg++;
			index = optarg;
			break;
		case 't':
			tflg++;
			table = optarg;
			break;
		default:
			errflg++;
			break;
		}

	/* validate command line options */
	if (errflg || !dflg || !tflg) {
		(void)fprintf(stderr, "usage: %s -d dbname -t tabname [-i idxname [value] | -h]\n", argv[0]);
		exit(1);
	}

	/* locate the database in the system */
	if (find_database() != SUCCESS) {
		(void)fprintf(stderr, "Database not found or no system permission.\n\n");
		exit(1);
	}

	/* locate the table in the database */
	if (read_systables() != SUCCESS) {
		(void)fprintf(stderr, "Table %s not found.\n", table);
		exit(1);
	}

	/* generate filename to open */
	if (sys_catalog())
		(void)sprintf(filename, "%s/%s", path, table);
	else {
		(void)sprintf(tmp_table, "%-7.7s%ld", table, ldlong(&systab[SYSTABLES_tabid]));
		for (c = 0; c < 10; c++)
			if (tmp_table[c] == ' ')
				tmp_table[c] = '_';
		(void)sprintf(&tmp_table[7 - (strlen(tmp_table) - 10)] , "%ld", ldlong(&systab[SYSTABLES_tabid]));
		(void)sprintf(filename, "%s/%s", path, tmp_table);
	}

	/* open the table */
	if ((fd = isopen(filename, ISINPUT + ISAUTOLOCK)) < SUCCESS)
		iserror();

	/* read column and index information */
	read_syscolumns();
	read_sysindexes();

	/* allocate some memory */
	if ((record = malloc((unsigned)ldint(&systab[SYSTABLES_rowsize]))) == NULL) {
		iserrno = 12;
		iserror();
	}
	if ((tmp_char = malloc((unsigned)(ldint(&systab[SYSTABLES_rowsize]) + 1))) == NULL) {
		iserrno = 12;
		iserror();
	}

	/* select appropriate index */
	if (argc > optind)
		value = argv[argc - 1];
	select_index(iflg);

	/* read & print C-ISAM header */
	if (isindexinfo(fd, &dict, 0) != SUCCESS)
		iserror();
	(void)printf("Number of keys defined: %d\n", ldint(&systab[SYSTABLES_nindexes]));
	(void)printf("Data record size: %d\n", ldint(&systab[SYSTABLES_rowsize]));
	(void)printf("Number of records in file: %ld\n\n", dict.di_nrecords);
	if (hflg)
		exit(0);

	/* Main Processing */
	if (argc > optind) {
		if (isread(fd, record, ISGTEQ) != SUCCESS)
			exit(0);
		(void)isread(fd, record, ISPREV);
	}
	SET(c);
	while (c) {
		(void)isread(fd, record, ISNEXT);
		switch (iserrno) {
		case SUCCESS:
			print_data();
			break;
		case ELOCKED:
			(void)fprintf(stderr, "***** record is locked *****\n");
			break;
		case EENDFILE:
			RESET(c);
			break;
		default:
			iserror();
		}
	}

	/* free allocated memory */
	free(record);
	free(tmp_char);
	free(syscol);
	free(sysind);
	free(dbview);

	/* close the table */
	if (isclose(fd) != SUCCESS)
		iserror();

	exit(0);
}


/*******************************************************************************
* This routine selects which index of a table is to be used and stuffs the row *
* with the user-specified search data.                                         *
*******************************************************************************/

select_index(iflg)
{
	char	testname[19];
	double	atof();
	int	i, column;
	long	atol(), tmp_long;
	struct keydesc key;
	dec_t	tmp_dec_t;

	/* was an index specified */
	if (iflg) {
		/* find the index name */
		for (i = 1; i <= ldint(&systab[SYSTABLES_nindexes]); i++) {
			ldchar(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_idxname], SYSINDEXES_owner - SYSINDEXES_idxname,
			     testname);
			if (strcmp(testname, index) == 0)
				break;
		}
		if (i == ldint(&systab[SYSTABLES_nindexes]) + 1) {
			(void)fprintf(stderr, "Index %s not found.\n", index);
			exit(1);
		}

		/* pre-stuff the data record */
		column = ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1]);
		switch (ldint(&syscol[(SYSCOLUMNS_SIZE * column) + SYSCOLUMNS_coltype]) & SQLTYPE) {
		case SQLCHAR:
			stchar(value, &record[dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwstart],
			    dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwlen);
			break;
		case SQLSMINT:
			stint((short)atoi(value), &record[dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwstart]);
			break;
		case SQLINT:
		case SQLSERIAL:
			stlong(atol(value), &record[dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwstart]);
			break;
		case SQLFLOAT:
			stdbl(atof(value), &record[dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwstart]);
			break;
		case SQLSMFLOAT:
			stfloat((float)atof(value), &record[dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwstart]);
			break;
		case SQLDECIMAL:
		case SQLMONEY:
			if (value[0] == '$')
				value[0] = ' ';
			if ((iserrno = deccvasc(value, strlen(value), &tmp_dec_t)) != SUCCESS) {
				(void)fprintf(stderr, "SQL statement error number %d.\n", iserrno);
				exit(1);
			}
			stdecimal(&tmp_dec_t, &record[dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwstart],
			     dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwlen);
			break;
		case SQLDATE:
			if ((iserrno = rstrdate(value, &tmp_long)) != SUCCESS) {
				(void)fprintf(stderr, "SQL statement error number %d.\n", iserrno);
				exit(1);
			}
			stlong(tmp_long, &record[dbview[ldint(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_part1])].vwstart]);
			break;
		}

		/* read index information */
		if (isindexinfo(fd, &key, i + 1) != SUCCESS)
			iserror();
	} else
		(void)memset(&key, 0, sizeof(key));

	/* select index for table */
	if (isstart(fd, &key, 0, record, ISFIRST) != SUCCESS)
		iserror();
}


/*******************************************************************************
* This routine prints individual columns in the row.                           *
*******************************************************************************/

print_data()
{
	char	tmp_char2[34];
	double	tmp_double;
	int	i;
	long	tmp_long;
	short	tmp_short;
	dec_t	tmp_dec_t;

	for (i = 1; i <= ldint(&systab[SYSTABLES_ncols]); i++) {
		switch (ldint(&syscol[(SYSCOLUMNS_SIZE*i)+SYSCOLUMNS_coltype]) & SQLTYPE) {
		case SQLCHAR:
			ldchar(&record[dbview[i].vwstart], dbview[i].vwlen, tmp_char);
			if (!risnull(SQLCHAR, tmp_char))
				(void)printf("%s", tmp_char);
			break;
		case SQLSMINT:
			tmp_short = ldint(&record[dbview[i].vwstart]);
			if (!risnull(SQLSMINT, (char *)&tmp_short))
				(void)printf("%d", tmp_short);
			break;
		case SQLINT:
		case SQLSERIAL:
			tmp_long = ldlong(&record[dbview[i].vwstart]);
			if (!risnull(SQLINT, (char *)&tmp_long))
				(void)printf("%d", tmp_long);
			break;
		case SQLFLOAT:
			tmp_double = lddbl(&record[dbview[i].vwstart]);
			if (!risnull(SQLFLOAT, (char *)&tmp_double))
				(void)printf("%f", tmp_double);
			break;
		case SQLSMFLOAT:
			tmp_double = ldfloat(&record[dbview[i].vwstart]);
			if (!risnull(SQLSMFLOAT, (char *)&tmp_double))
				(void)printf("%f", tmp_double);
			break;
		case SQLMONEY:
			(void)putchar('$');
		case SQLDECIMAL:
			(void)lddecimal(&record[dbview[i].vwstart], dbview[i].vwlen, &tmp_dec_t);
			if (!risnull(SQLDECIMAL, (char *)&tmp_dec_t)) {
				(void)dectoasc(&tmp_dec_t, tmp_char, ldint(&systab[SYSTABLES_rowsize]) + 1, dbview[i].vwscale);
				ldchar(tmp_char, ldint(&systab[SYSTABLES_rowsize]) + 1, tmp_char2);
				(void)printf("%s", tmp_char2);
			}
			break;
		case SQLDATE:
			tmp_long = ldlong(&record[dbview[i].vwstart]);
			if (!risnull(SQLDATE, (char *)&tmp_long)) {
				(void)rdatestr(tmp_long, tmp_char);
				(void)printf("%s", tmp_char);
			}
			break;
		}
		if (i != ldint(&systab[SYSTABLES_ncols]))
			(void)putchar(',');
	}
	(void)putchar('\n');
}


read_systables()
{
	char	sysfile[128], tmp_table[19];
	int	isfd;
	struct keydesc key;

	(void)sprintf(sysfile, "%s/systables", path);

	/* open SYSTABLES */
	if ((isfd = isopen(sysfile, ISINPUT + ISAUTOLOCK)) < SUCCESS)
		iserror();

	/* select key "tabname" */
	if (isindexinfo(isfd, &key, 1) != SUCCESS)
		iserror();
	if (isstart(isfd, &key, 0, systab, ISFIRST) != SUCCESS)
		iserror();

	/* define search value */
	stchar(table, &systab[SYSTABLES_tabname], SYSTABLES_owner - SYSTABLES_tabname);

	/* read SYSTABLES */
	if (isread(isfd, systab, ISGTEQ) != SUCCESS)
		return(-1);

	/* close SYSTABLES */
	if (isclose(isfd) != SUCCESS)
		iserror();

	/* confirm read value */
	ldchar(&systab[SYSTABLES_tabname], SYSTABLES_owner - SYSTABLES_tabname, tmp_table);
	if (strcmp(table, tmp_table) != 0)
		return(-1);

	return(0);
}


read_syscolumns()
{
	char	sysfile[128];
	int	i, isfd, bytecount = 0;
	struct keydesc key;

	(void)sprintf(sysfile, "%s/syscolumns", path);
	if ((syscol = calloc((unsigned)(ldint(&systab[SYSTABLES_ncols]) + 1), (unsigned)SYSCOLUMNS_SIZE)) == NULL) {
		iserrno = 12;
		iserror();
	}
	if ((dbview = (struct DBVIEW *)calloc((unsigned)(ldint(&systab[SYSTABLES_ncols]) + 1), (unsigned)sizeof(struct DBVIEW ))) ==
	    NULL) {
		iserrno = 12;
		iserror();
	}

	/* open SYSCOLUMNS */
	if ((isfd = isopen(sysfile, ISINPUT + ISAUTOLOCK)) < SUCCESS)
		iserror();

	/* select key "column" */
	if (isindexinfo(isfd, &key, 1) != SUCCESS)
		iserror();
	if (isstart(isfd, &key, 0, &syscol[SYSCOLUMNS_SIZE * 1], ISFIRST) != SUCCESS)
		iserror();

	/* read SYSCOLUMNS */
	for (i = 1; i <= ldint(&systab[SYSTABLES_ncols]); i++) {
		(void)memcpy(&syscol[(SYSCOLUMNS_SIZE * i) + SYSCOLUMNS_tabid], &systab[SYSTABLES_tabid], LONGSIZE);
		stint(i, &syscol[(SYSCOLUMNS_SIZE * i) + SYSCOLUMNS_colno]);
		if (isread(isfd, &syscol[SYSCOLUMNS_SIZE * i], ISEQUAL) != SUCCESS)
			iserror();
		if ((ldint(&syscol[(SYSCOLUMNS_SIZE * i) + SYSCOLUMNS_coltype]) & SQLTYPE) == SQLDECIMAL || 
		    (ldint(&syscol[(SYSCOLUMNS_SIZE * i) + SYSCOLUMNS_coltype]) & SQLTYPE) == SQLMONEY) {
			dbview[i].vwprec = PRECTOT(ldint(&syscol[(SYSCOLUMNS_SIZE * i) + SYSCOLUMNS_collength]));
			dbview[i].vwscale = PRECDEC(ldint(&syscol[(SYSCOLUMNS_SIZE * i) + SYSCOLUMNS_collength]));
			dbview[i].vwlen = DECLEN(dbview[i].vwprec, dbview[i].vwscale);
		} else
			dbview[i].vwlen = ldint(&syscol[(SYSCOLUMNS_SIZE * i) + SYSCOLUMNS_collength]);
		dbview[i].vwstart = bytecount;
		bytecount += dbview[i].vwlen;
	}

	/* close SYSCOLUMNS */
	if (isclose(isfd) != SUCCESS)
		iserror();
}


read_sysindexes()
{
	char	sysfile[128];
	int	i = 1, isfd;
	struct keydesc key;

	(void)sprintf(sysfile, "%s/sysindexes", path);
	if ((sysind = calloc((unsigned)(ldint(&systab[SYSTABLES_nindexes]) + 1), (unsigned)SYSINDEXES_SIZE)) == NULL) {
		iserrno = 12;
		iserror();
	}

	/* Does the table have indexes */
	if (ldint(&systab[SYSTABLES_nindexes]) == 0)
		return;

	/* open SYSINDEXES */
	if ((isfd = isopen(sysfile, ISINPUT + ISAUTOLOCK)) < SUCCESS)
		iserror();

	/* select key "idxtab" */
	if (isindexinfo(isfd, &key, 1) != SUCCESS )
		iserror();
	if (isstart(isfd, &key, 0, &sysind[SYSINDEXES_SIZE*1], ISFIRST) != SUCCESS)
		iserror();

	/* define search value */
	(void)memcpy(&sysind[(SYSINDEXES_SIZE * i) + SYSINDEXES_tabid], &systab[SYSTABLES_tabid], LONGSIZE);

	/* read SYSINDEXES */
	if (isread(isfd, &sysind[SYSINDEXES_SIZE * i], ISEQUAL) != SUCCESS)
		iserror();
	while (++i <= ldint(&systab[SYSTABLES_nindexes]))
		if (isread(isfd, &sysind[SYSINDEXES_SIZE * i], ISNEXT) != SUCCESS)
			iserror();

	if (isclose(isfd) != SUCCESS)
		iserror();
}


/*******************************************************************************
* This function will search the DBPATH environmental variable to find the      *
* named database.                                                              *
*******************************************************************************/

find_database()
{
	char	*getenv(), *dbpath = NULL, *s;

	dbpath = getenv("DBPATH");
	if (dbpath == NULL)
		dbpath = ".";

	if (database[0] == '/') {
		(void)sprintf(path, "%s.dbs", database);
		return(access(path, 0));
	}

	s = strtok(dbpath, ":\n");
	(void)sprintf(path, "%s/%s.dbs", s, database);
	if (access(path, 0) == SUCCESS)
		return(0);

	while ((s = strtok(NULL, ":\n")) != NULL) {
		(void)sprintf(path, "%s/%s.dbs", s, database);
		if (access(path, 0) == SUCCESS)
			return(0);
	}

	return(-1);
}


/*******************************************************************************
* This routine will print out a nice error message and then do an exit(1).     *
*******************************************************************************/

iserror()
{
	extern char	*sys_errlist[];
	extern int	sys_nerr;
	void	exit();

	if (iserrno >= 100 && iserrno <= is_nerr)
		(void)fprintf(stderr, "C-ISAM error %d: %s\n", iserrno, is_errlist[iserrno-100]);
	else if (iserrno >= 1 && iserrno <= sys_nerr)
		(void)fprintf(stderr, "System error %d: %s\n", iserrno, sys_errlist[iserrno]);
	else
		(void)fprintf(stderr, "Unknown error %d: internal error\n", iserrno);

	exit(1);
}


/*******************************************************************************
* This function determines if the specified table is a system table.           *
*******************************************************************************/

char	*catalogs[] = { "systables", "syscolumns", "sysindexes", "systabauth", "syscolauth", "sysdepend", "syssynonyms", "sysusers",
	     "sysviews", NULL};


sys_catalog()
{
	int	i = 0;

	while (catalogs[i] != NULL)
		if (strcmp(table, catalogs[i++]) == 0)
			return(1);

	return(0);
}


