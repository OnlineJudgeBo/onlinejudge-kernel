#include "models.h"
#include "../shared.h"
#include "similar_code.h"
#include <stdio.h>
#include <string.h>

Similar_Code get_similar_code(int solution_id, int lang, int p_id,
                              int contest_id, char *work_dir) {
  char cmd[BUFFER_SIZE];
  int written =
      snprintf(cmd, sizeof(cmd), "/usr/bin/anti_cheating.sh %s %d %d .%s %d",
               oj_home, solution_id, contest_id, lang_ext[lang], p_id);

  if (written < 0) {
    write_log("Error generating work_dir with snprintf. similar_code0\n");
  }

  int first_number;
  int second_number = 0;
  double third_number = 0.0;
  char buffer[BUFFER_SIZE] = "";
  char output[BUFFER_SIZE] = "";

  FILE *fjobs = read_cmd_output("%s", cmd);

  while (fgets(buffer, BUFFER_SIZE, fjobs) != NULL) {
    strcat(output, buffer);
  }
  pclose(fjobs);

  if (sscanf(output, "%d%*[^,],%d%*[^,],%lf", &first_number, &second_number,
             &third_number) != 3) {
    printf("Error reading command output\n");
    printf("%s", output);
  }

  Similar_Code similar;
  similar.similar_s_id = second_number;
  similar.percentage = (int)(third_number * 100);

  if (similar.percentage < 70) {
    similar.percentage = 0;
  }
  return similar;
}
