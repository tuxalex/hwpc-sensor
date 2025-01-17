/*
 *  Copyright (c) 2018, INRIA
 *  Copyright (c) 2018, University of Lille
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include <string.h>

#include "target.h"
#include "target_kubernetes.h"

/*
 * CONTAINER_ID_REGEX is the regex used to extract the Docker container id from a cgroup path.
 * CONTAINER_ID_REGEX_EXPECTED_MATCHES is the number of matches expected from the regex. (num groups + 1)
 */
#define CONTAINER_ID_REGEX \
    "perf_event/kubepods.slice/" \
    "kubepods-(besteffort|burstable|).slice/" \
    "kubepods-(besteffort|burstable|)-(pod[a-zA-Z0-9][a-zA-Z0-9._]+).slice/" /* Pod ID */ \
    "cri-containerd-([a-f0-9]{64}).scope" /* Container ID */
#define CONTAINER_ID_REGEX_EXPECTED_MATCHES 5

#define CONTAINER_SHORT_ID_LENGTH 14

/*
 * CONTAINER_NAME_REGEX is the regex used to extract the name of the Docker container from its json configuration file.
 * CONTAINER_NAME_REGEX_EXPECTED_MATCHES is the number of matches expected from the regex. (num groups + 1)
 */
//#define CONTAINER_NAME_REGEX "\"hostname\":\"/([a-zA-Z0-9][a-zA-Z0-9_.-]+)\""
//#define CONTAINER_NAME_REGEX_EXPECTED_MATCHES 2


int
target_kubernetes_validate(const char *cgroup_path)
{
    regex_t re;
    bool is_kubernetes_target = false;

    if (!cgroup_path)
        return false;

    if (!regcomp(&re, CONTAINER_ID_REGEX, REG_EXTENDED | REG_NEWLINE | REG_NOSUB)) {
        if (!regexec(&re, cgroup_path, 0, NULL, 0))
            is_kubernetes_target = true;

        regfree(&re);
    }

    return is_kubernetes_target;
}

char *
target_kubernetes_container_id(char *cgroup_path)
{
    regex_t re;
    regmatch_t matches[CONTAINER_ID_REGEX_EXPECTED_MATCHES];
    char *id = NULL;
    char *target_name;
    target_name = (char *) malloc ((CONTAINER_SHORT_ID_LENGTH + 1) * sizeof (char));

    if (!regcomp(&re, CONTAINER_ID_REGEX, REG_EXTENDED | REG_NEWLINE)) {
        if (!regexec(&re, cgroup_path, CONTAINER_ID_REGEX_EXPECTED_MATCHES, matches, 0)) {
            id = cgroup_path + matches[4].rm_so;
            strncpy(target_name, id, CONTAINER_SHORT_ID_LENGTH);
            target_name[CONTAINER_SHORT_ID_LENGTH] = '\0';
        }
        regfree(&re);
    }

    return target_name;
}


// static char *
// build_container_config_path(const char *cgroup_path)
// {
//     regex_t re;
//     regmatch_t matches[CONTAINER_ID_REGEX_EXPECTED_MATCHES];
//     regoff_t length;
//     const char *id = NULL;
//     char buffer[PATH_MAX] = {0};
//     char *config_path = NULL;

//     if (!regcomp(&re, CONTAINER_ID_REGEX, REG_EXTENDED | REG_NEWLINE)) {
//         if (!regexec(&re, cgroup_path, CONTAINER_ID_REGEX_EXPECTED_MATCHES, matches, 0)) {
//             id = cgroup_path + matches[4].rm_so;
//             length = matches[4].rm_eo - matches[4].rm_so;
//             snprintf(buffer, PATH_MAX, "/run/containerd/io.containerd.runtime.v2.task/k8s.io/%.*s/config.json", length, id);
//             config_path = strdup(buffer);
//         }
//         regfree(&re);
//     }

//     return config_path;
// }

// char *
// target_kubernetes_resolve_name(struct target *target)
// {
//     char *config_path = NULL;
//     FILE *json_file = NULL;
//     char *json = NULL;
//     size_t json_len;
//     regex_t re;
//     regmatch_t matches[CONTAINER_NAME_REGEX_EXPECTED_MATCHES];
//     char *target_name = NULL;

//     config_path = build_container_config_path(target->cgroup_path);
//     if (!config_path) {
//         zsys_warning("perf<none>: container config file path is empty. Found container name from container id is not possible.");
//         return NULL;
//     }

//     json_file = fopen(config_path, "r");
//     if (json_file) {
//         if (getline(&json, &json_len, json_file) != -1) {
//             if (!regcomp(&re, CONTAINER_NAME_REGEX, REG_EXTENDED | REG_NEWLINE)) {
//                 if (!regexec(&re, json, CONTAINER_NAME_REGEX_EXPECTED_MATCHES, matches, 0))
//                     target_name = strndup(json + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);

//                 regfree(&re);
//             }
//             free(json);
//         }
//         fclose(json_file);
//     }
//     else {
//       zsys_warning("perf<none>: container config file cannot be open. Found container name from container id is not possible.");
//       free(config_path);
//       return NULL;
//     }

//     free(config_path);
//     return target_name;
// }

