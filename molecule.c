#include "molecule.h"
#include "argument.h"

static int *n_mlc;
static struct mlc_t **mlc;
static FILE **fi, **fi_filter;

void mlc_init(int n_target, char **target_name)
{
	int i;
	__CALLOC(n_mlc, n_target);
	__CALLOC(mlc, n_target);
	__CALLOC(fi, n_target);
	__CALLOC(fi_filter, n_target);
	char file_path[BUFSZ];
	for (i = 0; i < n_target; ++i) {
		sprintf(file_path, "%s/temp.%s.mlc.tsv", args.out_dir, target_name[i]);
		fi[i] = fopen(file_path, "w");
		sprintf(file_path, "%s/temp.filter.%s.mlc.tsv", args.out_dir, target_name[i]);
		fi_filter[i] = fopen(file_path, "w");
	}
}

void mlc_destroy(int n_target, char **target_name)
{
	int i, j;
	for (i = 0; i < n_target; ++i) {
		for (j = 0; j < n_mlc[i]; ++j)
			free(mlc[i][j].bar_s);
		free(mlc[i]);
		fclose(fi[i]);
		fclose(fi_filter[i]);
	}
	__FREE_AND_NULL(mlc);
	__FREE_AND_NULL(n_mlc);
}

static void mlc_out(int id, struct mlc_t *memb, char *bar_s, int start, int end)
{
	int len = end - start;
	fprintf(fi[id], "%d\t%d\t%d\t%s\t%d\t%.2f\n",
		start, end, len, bar_s, memb->sz, 1.0 * memb->sum_len / len);
}

static void mlc_filter(int id, struct mlc_t *memb, char *bar_s, int start, int end)
{
	int len = end - start;
	fprintf(fi_filter[id], "%d\t%d\t%d\t%s\t%d\t%.2f\n",
		start, end, len, bar_s, memb->sz, 1.0 * memb->sum_len / len);
}

void mlc_insert(int bx_id, bam1_t *b, struct stats_t *stats)
{
	if (bx_id == -1)
		return;

	int *n = &n_mlc[stats->id];
	struct mlc_t **p = &mlc[stats->id];
	int pos = b->core.pos, len = b->core.l_qseq;
	uint8_t *tag_data = bam_aux_get(b, "BX");
	char *bar_s = bam_aux2Z(tag_data);
	assert(bar_s);

	if (bx_id >= *n) {
		int old_sz = *n;
		*n = bx_id + 1;
		__REALLOC(*p, *n);
		memset(*p + old_sz, 0, (*n - old_sz) * sizeof(struct mlc_t));
	}

	struct mlc_t *memb = *p + bx_id;
	int start, end;

	if (memb->sz && memb->end < pos - MLC_CONS_THRES) {
		end = memb->end + memb->last_len;
		start = memb->start;
		if (memb->sz >= args.thres_read_mlc &&
		    end - start >= args.thres_len_mlc)
			mlc_out(stats->id, memb, memb->bar_s, start, end);
		else
			mlc_filter(stats->id, memb, memb->bar_s, start, end);
		free(memb->bar_s);
		memb->sz = 1;
		memb->start = memb->end = pos;
		memb->sum_len = len;
		memb->last_len = len;
		memb->bar_s = strdup(bar_s);
	} else {
		++memb->sz;
		memb->end = pos;
		memb->last_len = len;
		memb->sum_len += len;
		if (memb->sz == 1) {
			memb->start = pos;
			memb->bar_s = strdup(bar_s);
		}
	}
}

void mlc_get_last(struct stats_t *stats)
{
	int n = n_mlc[stats->id];
	struct mlc_t *p = mlc[stats->id];
	int i, start, end;

	for (i = 0; i < n; ++i) {
		struct mlc_t *memb = p + i;
		if (memb->sz) {
			end = memb->end + memb->last_len;
			start = memb->start;
			if (memb->sz >= args.thres_read_mlc &&
			    end - start >= args.thres_len_mlc)
				mlc_out(stats->id, memb, memb->bar_s, start, end);
			else
				mlc_filter(stats->id, memb, memb->bar_s, start, end);
			free(memb->bar_s);
			memb->bar_s = NULL;
		}
	}
}

void mlc_fetch(struct stats_t *stats, int pos)
{
	int n = n_mlc[stats->id];
	struct mlc_t *p = mlc[stats->id];
	int i, start, end;

	for (i = 0; i < n; ++i) {
		struct mlc_t *memb = p + i;
		if (memb->sz && memb->end + MLC_CONS_THRES < pos) {
			end = memb->end + memb->last_len;
			start = memb->start;
			if (memb->sz >= args.thres_read_mlc &&
			    end - start >= args.thres_len_mlc)
				mlc_out(stats->id, memb, memb->bar_s, start, end);
			else
				mlc_filter(stats->id, memb, memb->bar_s, start, end);
			free(memb->bar_s);
			memb->bar_s = NULL;
			memb->sz = 0;
		}
	}
}
