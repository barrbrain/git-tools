#include <stdio.h>
#include <string.h>

#include "git2.h"

static int foreach_cb(const git_oid *oid, void *data)
{
	git_repository *repo = (git_repository *)data;
	git_odb *odb;
	git_otype otype;
	size_t osize;
	char ohex[41];

	git_repository_odb(&odb, repo);
	git_odb_read_header(&osize, &otype, odb, oid);
	git_odb_free(odb);
	git_oid_fmt(ohex, oid);
	ohex[40] = '\0';

	printf("%s %s %zu\n", ohex, git_object_type2string(otype), osize);

        return 0;
}

int main(int argc, char** argv) {
	git_odb *_odb = NULL;
	git_repository *_repo = NULL;
	char buf[4096];

	for(fgets(buf, sizeof(buf), stdin); !feof(stdin); fgets(buf, sizeof(buf), stdin)) {
		if (strlen(buf) && buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		if (!git_repository_open(&_repo, buf)) {
			printf("%s\n", buf);
			git_repository_odb(&_odb, _repo);
			git_odb_foreach(_odb, &foreach_cb, _repo);
		} else {
			fprintf(stderr, "failed: %s\n", buf);
		}
		if (_odb) {
			git_odb_free(_odb);
			_odb = NULL;
		}
		if (_repo) {
			git_repository_free(_repo);
			_repo = NULL;
		}
	}
	return 0;
}
