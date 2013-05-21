/*	$NetBSD: rpc_tblout.c,v 1.12 2011/08/31 16:24:58 plunky Exp $	*/
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user or with the express written consent of
 * Sun Microsystems, Inc.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
#if 0
static char sccsid[] = "@(#)rpc_tblout.c 1.4 89/02/22 (C) 1988 SMI";
#else
__RCSID("$NetBSD: rpc_tblout.c,v 1.12 2011/08/31 16:24:58 plunky Exp $");
#endif
#endif

/*
 * rpc_tblout.c, Dispatch table outputter for the RPC protocol compiler
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rpc_scan.h"
#include "rpc_parse.h"
#include "rpc_util.h"

#define TABSIZE		8
#define TABCOUNT	5
#define TABSTOP		(TABSIZE*TABCOUNT)

static char tabstr[TABCOUNT + 1] = "\t\t\t\t\t";

static const char tbl_hdr[] = "struct rpcgen_table %s_table[] = {\n";
static const char tbl_end[] = "};\n";

static const char null_entry[] = "\t(char *(*)())0,\n\
 \t(xdrproc_t)xdr_void,\t\t0,\n\
 \t(xdrproc_t)xdr_void,\t\t0,\n";

static const char tbl_nproc[] =
    "u_int %s_nproc =\n\t(u_int)(sizeof(%s_table)/sizeof(%s_table[0]));\n\n";

static void write_table __P((definition *));
static void printit __P((char *, char *));

void
write_tables()
{
	list   *l;
	definition *def;

	f_print(fout, "\n");
	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			write_table(def);
		}
	}
}

static void
write_table(def)
	definition *def;
{
	version_list *vp;
	proc_list *proc;
	int     current;
	int     expected;
	char    progvers[100];
	int     warning;

	for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
		warning = 0;
		s_print(progvers, "%s_%s",
		    locase(def->def_name), vp->vers_num);
		/* print the table header */
		f_print(fout, tbl_hdr, progvers);

		if (nullproc(vp->procs)) {
			expected = 0;
		} else {
			expected = 1;
			f_print(fout, null_entry);
		}
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			if (expected != 0)
				f_print(fout, "\n");
			current = atoi(proc->proc_num);
			if (current != expected++) {
				f_print(fout,
				    "/*\n * WARNING: table out of order\n */\n\n");
				if (warning == 0) {
					f_print(stderr,
					    "WARNING %s table is out of order\n",
					    progvers);
					warning = 1;
					nonfatalerrors = 1;
				}
				expected = current + 1;
			}
			f_print(fout, "\t(char *(*)())RPCGEN_ACTION(");

			/* routine to invoke */
			if (!newstyle)
				pvname_svc(proc->proc_name, vp->vers_num);
			else {
				if (newstyle)
					f_print(fout, "_");	/* calls internal func */
				pvname(proc->proc_name, vp->vers_num);
			}
			f_print(fout, "),\n");

			/* argument info */
			if (proc->arg_num > 1)
				printit(NULL, proc->args.argname);
			else
				/* do we have to do something special for
				 * newstyle */
				printit(proc->args.decls->decl.prefix,
				    proc->args.decls->decl.type);
			/* result info */
			printit(proc->res_prefix, proc->res_type);
		}

		/* print the table trailer */
		f_print(fout, tbl_end);
		f_print(fout, tbl_nproc, progvers, progvers, progvers);
	}
}

static void
printit(prefix, type)
	char   *prefix;
	char   *type;
{
	int     len;
	int     tabs;


	len = fprintf(fout, "\txdr_%s,", stringfix(type));
	/* account for leading tab expansion */
	len += TABSIZE - 1;
	/* round up to tabs required */
	tabs = (TABSTOP - len + TABSIZE - 1) / TABSIZE;
	f_print(fout, "%s", &tabstr[TABCOUNT - tabs]);

	if (streq(type, "void")) {
		f_print(fout, "0");
	} else {
		f_print(fout, "(u_int)sizeof(");
		/* XXX: should "follow" be 1 ??? */
		ptype(prefix, type, 0);
		f_print(fout, ")");
	}
	f_print(fout, ",\n");
}