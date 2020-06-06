#ifndef DUMPER_H_INCLUDED
#define DUMPER_H_INCLUDED

struct g_thread;

/* 手抜き表示 */
void dump_atoms_raw(struct g_thread*);

/* 全アトムを表示 */
void dump_atoms(struct g_thread*);

#endif
