#include <stdio.h>
#include <string.h>

#include "git2.h"

#include "oidmap.h"

GIT__USE_OIDMAP;

static git_oidmap *oid_map;

static int foreach_cb(git_oid *oid, void *data)
{
	git_repository *repo = (git_repository *)data;
	git_tree *tree;
	const git_tree_entry *entry;

	/* ignore non-tree objects */
	if (git_tree_lookup(&tree, repo, oid))
		return 0;
	
	int cnt = git_tree_entrycount(tree);
	while (cnt--) {
		int pos;
		const git_oid *entry_oid;
		entry = git_tree_entry_byindex(tree, cnt);
		entry_oid = git_tree_entry_id(entry);

		if ((pos = kh_get(oid, oid_map, entry_oid)) != kh_end(oid_map)) {
			git_odb *odb;
			git_otype otype;
			size_t osize;
			char ohex[41];
			const char *oname = git_tree_entry_name(entry);
			
			git_repository_odb(&odb, repo);
			git_odb_read_header(&osize, &otype, odb, entry_oid);
			git_oid_fmt(ohex, entry_oid);
			ohex[40] = '\0';

			printf("%s %s %zu %s\n", ohex, git_object_type2string(otype), osize, oname);
			kh_del(oid, oid_map, pos);
		}
	}	
	git_tree_free(tree);

        return 0;
}

int main(int argc, char** argv) {
	git_odb *_odb = NULL;
	git_repository *_repo = NULL;
	char buf[4096];
	const size_t oid_buf_asize = 1 << 24;
	size_t oid_buf_size = 0;
	git_oid *oid_buf = malloc(oid_buf_asize * sizeof(git_oid));
	
	oid_map = git_oidmap_alloc();
        GITERR_CHECK_ALLOC(oid_map);

	for(fgets(buf, sizeof(buf), stdin); !feof(stdin); fgets(buf, sizeof(buf), stdin)) {
		if (strlen(buf) && buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		if (*buf == '/') {
			char *delim = strstr(buf, "/objects");
			if (delim) {
				*delim = '\0';
				if (!git_repository_open(&_repo, buf)) {
					printf("%s\n", buf);
					git_repository_odb(&_odb, _repo);
					git_odb_foreach(_odb, &foreach_cb, _repo);
				} else {
					fprintf(stderr, "failed: %s\n", buf);
				}
			}
			if (_repo) {
				git_repository_free(_repo);
				_repo = NULL;
				_odb = NULL;
			}
			git_oidmap_free(oid_map);
			oid_map = git_oidmap_alloc();
			GITERR_CHECK_ALLOC(oid_map);
			oid_buf_size = 0;
		} else {
			int ret;
			int pos;
			git_oid *oid;
			if (oid_buf_size == oid_buf_asize)
				continue;
			oid = &oid_buf[oid_buf_size++];
			git_oid_fromstr(oid, buf);
			pos = kh_put(oid, oid_map, oid, &ret);
			kh_value(oid_map, pos) = "";
		}
	}
	return 0;
}
