/*
    sqltabs.h - header file for programs using Informix-SQL system tables.
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


#define SYSTABLES_tabname  0	/* index #1, part 1 */
#define SYSTABLES_owner    18	/* index #1, part 2 */
#define SYSTABLES_dirpath  26
#define SYSTABLES_tabid    90	/* index #2 */
#define SYSTABLES_rowsize  94
#define SYSTABLES_ncols    96
#define SYSTABLES_nindexes 98
#define SYSTABLES_nrows    100
#define SYSTABLES_created  104	/* julian date */
#define SYSTABLES_version  108
#define SYSTABLES_tabtype  112	/* <T>able, <V>iew, <L>og */
#define SYSTABLES_audpath  113
#define SYSTABLES_SIZE     177

#define SYSCOLUMNS_colname   0
#define SYSCOLUMNS_tabid     18	/* index #1, part1 */
#define SYSCOLUMNS_colno     22	/* index #1, part2 */
#define SYSCOLUMNS_coltype   24
#define SYSCOLUMNS_collength 26
#define SYSCOLUMNS_SIZE      28

#define SYSINDEXES_idxname   0	/* index #2, part1 */
#define SYSINDEXES_owner     18
#define SYSINDEXES_tabid     26	/* index #1, part1 */ /* index #2, part2 */
#define SYSINDEXES_idxtype   30	/* <U>nique, <D>uplicates */
#define SYSINDEXES_clustered 31
#define SYSINDEXES_part1     32
#define SYSINDEXES_part2     34
#define SYSINDEXES_part3     36
#define SYSINDEXES_part4     38
#define SYSINDEXES_part5     40
#define SYSINDEXES_part6     42
#define SYSINDEXES_part7     44
#define SYSINDEXES_part8     46
#define SYSINDEXES_SIZE      48

#define SYSTABAUTH_grantor 0	/* index #1, part2 */
#define SYSTABAUTH_grantee 8	/* index #1, part3 */ /* index #2, part2 */
#define SYSTABAUTH_tabid   16	/* index #1, part1 */ /* index #2, part1 */
#define SYSTABAUTH_tabauth 20
#define SYSTABAUTH_SIZE    27

#define SYSCOLAUTH_grantor 0	/* index #1, part2 */
#define SYSCOLAUTH_grantee 8	/* index #1, part3 */ /* index #2, part2 */
#define SYSCOLAUTH_tabid   16 	/* index #1, part1 */ /* index #2, part1 */
#define SYSCOLAUTH_colno   20	/* index #1, part4 */
#define SYSCOLAUTH_colauth 22
#define SYSCOLAUTH_SIZE    24

#define SYSDEPEND_btabid 0	/* index #1, part1 */
#define SYSDEPEND_btype  2
#define SYSDEPEND_dtabid 3	/* index #2, part1 */
#define SYSDEPEND_dtype  5
#define SYSDEPEND_SIZE   6

#define SYSSYNONYMS_owner   0	/* index #1, part1 */
#define SYSSYNONYMS_synonym 8	/* index #1, part2 */
#define SYSSYNONYMS_created 26	/* julian date */
#define SYSSYNONYMS_tabid   30	/* index #2, part1 */
#define SYSSYNONYMS_SIZE    34

#define SYSUSERS_username 0	/* index #1, part1 */
#define SYSUSERS_usertype 8	/* <D>ba, <R>esource, <C>onnect */
#define SYSUSERS_priority 9
#define SYSUSERS_password 11
#define SYSUSERS_SIZE     19

#define SYSVIEWS_tabid 0	/* index #1, part1 */
#define SYSVIEWS_seqno 4	/* index #1, part2 */
#define SYSVIEWS_text  6
#define SYSVIEWS_SIZE  70
