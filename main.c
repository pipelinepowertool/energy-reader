#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct PidInfo {
    char pid[10];
    long total;
};

char *concat(const char *string, const char *string1);

bool checkRAPLSupport(const char *directory, const char *fileName);

char *readFile(const char *file);

void calculateCpuCycles(long *busy, long *total);

long calculateCpuCyclesPid(char *pid);

char *str_replace(const char *orig, char *rep, char *with);

void setPidArray(struct PidInfo *list1[], struct PidInfo *list2[], int *pidAmount);

void updatePidArray(struct PidInfo *list[], int *pidAmount);

void calculateEnergy(long *cpuEnergy);

const char pkgDir[] = "/Users/sdenboer/code/scriptie/rapl/intel-rapl:0/";
//const char pkgDir[] = "/sys/class/powercap/intel-rapl/intel-rapl:0/";
const char dramDir[] = "/Users/sdenboer/code/scriptie/rapl/intel-rapl:0/intel-rapl:0:2/";
//const char dramDir[] = "/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:2/";
const char psysDir[] = "/Users/sdenboer/code/scriptie/rapl/intel-rapl:1/";
//const char psysDir[] = "/sys/class/powercap/intel-rapl/intel-rapl:1/";
const char cpuCyclesFile[] = "/Users/sdenboer/code/scriptie/proc/stat";
//const char cpuCyclesFile[] = "/proc/stat";
const char cpuCyclesPidFile[] = "/Users/sdenboer/code/scriptie/proc/[pid]/stat";
//const char cpuCyclesPidFile[] = "/proc/[pid]/stat";
const char command[] = "ps -o pid= | xargs | tr -d '\n'";
//const char command[] = "ps -efho pid -t ? | xargs | tr -d '\n'";

bool pkgSupported;
bool dramSupported;
bool psysSupported;


int main() {
    long cpuCycleBusy_1;
    long cpuCycleTotal_1;
    long cpuEnergy_1;

    struct PidInfo *pidInfoList_1;
    struct PidInfo *pidInfoList_2;
    int pidAmount;

    long cpuCycleBusy_2;
    long cpuCycleTotal_2;
    long cpuEnergy_2;
    pkgSupported = checkRAPLSupport(pkgDir, "package-0");
    dramSupported = checkRAPLSupport(dramDir, "package-0");
    psysSupported = checkRAPLSupport(psysDir, "psys");

    if (!pkgSupported && !dramSupported && !psysSupported) {
        printf("Custom energy model not supported yet\n");
        exit(1);
    }

    calculateCpuCycles(&cpuCycleBusy_1, &cpuCycleTotal_1);

    setPidArray(&pidInfoList_1, &pidInfoList_2, &pidAmount);

    calculateEnergy(&cpuEnergy_1);

    sleep(1);

    calculateCpuCycles(&cpuCycleBusy_2, &cpuCycleTotal_2);
    updatePidArray(&pidInfoList_2, &pidAmount);
    calculateEnergy(&cpuEnergy_2);

    float total = ((float) cpuCycleTotal_2 - (float) cpuCycleTotal_1);

    float cpuUtil = total != 0 ? (float) cpuCycleBusy_2 - (float) cpuCycleBusy_1 / total : 0;
    long cpuPower = cpuEnergy_2 - cpuEnergy_1;

    for (int i = 0; i < pidAmount; ++i) {
        long timed = pidInfoList_2[i].total - pidInfoList_1[i].total;
        float cpuPidUtil = total != 0 ? (float) timed / total : 0;
        float cpuPidPower = cpuUtil != 0 ? ((cpuPidUtil * cpuPower) / cpuUtil) : 0;
        printf("%f\n", cpuPidPower);
    }

    printf("%d", cpuPower);


    free(pidInfoList_1);
//    free(pidInfoList_2);
    return 0;
}

void setPidArray(struct PidInfo *list1[], struct PidInfo *list2[], int *pidAmount) {
    FILE *fp;
    int i, count = 0;
    fp = popen(command, "r"); //MAC
    char buffer[255];
    char *s = fgets(buffer, 255, fp);
    fclose(fp);
    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] == ' ' && s[i + 1] != ' ')
            count++;
    }

    *pidAmount = count;

    *list1 = malloc((count) * sizeof(struct PidInfo));
    *list2 = malloc((count) * sizeof(struct PidInfo));

    int j = 0;
    char *token = strtok(s, " ");
    while (token != NULL) {
        strncpy((*list1)[j].pid, token, 10);
        strncpy((*list2)[j].pid, token, 10);
        (*list1)[j].total = calculateCpuCyclesPid(token);
        token = strtok(NULL, " ");
        j++;
    }

}

void updatePidArray(struct PidInfo *list[], int *pidAmount) {
    for (int k = 0; k < *pidAmount; ++k) {
        (*list)[k].total = calculateCpuCyclesPid((*list)[k].pid);
    }

}


void calculateCpuCycles(long *busy, long *total) {
    FILE *fp;
    fp = fopen(cpuCyclesFile, "r");
    long user, nice, system, idle;

    fscanf(fp, "%*s %ld %ld %ld %ld %*d %*d %*d %*d %*d %*d", &user, &nice, &system, &idle);

    *busy = (user + nice + system);
    *total = (user + nice + system + idle);
    fclose(fp);
}

void calculateEnergy(long *cpuEnergy) {
    FILE *fp;
    long microjoules;
    if (psysSupported) {
        fp = fopen(concat(psysDir, "energy_uj"), "r");
        fscanf(fp, "%ld", &microjoules);
        fclose(fp);
    } else if (pkgSupported) {
        fp = fopen(concat(pkgDir, "energy_uj"), "r");
        fscanf(fp, "%ld", &microjoules);
        if (dramSupported) {
            long dramMicroJoules;
            fp = fopen(concat(dramDir, "energy_uj"), "r");
            fscanf(fp, "%ld", &dramMicroJoules);
            microjoules += dramMicroJoules;
        }
        fclose(fp);
    } else {
        exit(0);
    }
    long j = (microjoules / 1000000);
    *cpuEnergy = j;
}

long calculateCpuCyclesPid(char *pid) {
    char *fileName = str_replace(cpuCyclesPidFile, "[pid]", pid);
    FILE *fp;
    fp = fopen(fileName, "r");
    long user, nice, system, idle;
    if (fp == NULL) {
        return 0;
    }
    fscanf(fp,
           "%*d %*s %*s %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld %ld %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d",
           &user, &system);
    fclose(fp);
    return (user + system);
}


bool checkRAPLSupport(const char *directory, const char *fileName) {
    char *file = concat(directory, "name");
    char *line = readFile(file);
    if (line == NULL) {
        return false;
    }
    char *ptr = strstr(line, fileName);
    if (ptr != NULL) {
        return true;
    }
    free(line);
    return false;
}

char *readFile(const char *file) {
    const int bufferLength = 300;
    char* line = malloc(bufferLength * sizeof(char));
//    char line[bufferLength];
    FILE *filePointer;

    filePointer = fopen(file, "r");
    if (filePointer == NULL) {
        return NULL;
    }
    fgets(line, bufferLength, filePointer);
    fclose(filePointer);
    return line;
}

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char *str_replace(const char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}
