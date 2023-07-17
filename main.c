#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t stop = 0;

void handle_int(int sig) {
    stop = 1;
}

struct PidInfo {
    char pid[10];
    char command[10000];
    long total_1;
    long total_2;
};

char *concat(const char *string, const char *string1);

bool checkRAPLSupport(const char *directory, const char *fileName);

char *readFile(const char *file);

void calculateCpuCycles(long *busy, long *total);

long calculateCpuCyclesPid(char *pid);

char *str_replace(const char *orig, char *rep, char *with);

void setPidArray(struct PidInfo *list1[], int *pidAmount);

void updatePidArray(struct PidInfo *list[], int *pidAmount);

void calculateEnergy(float *cpuEnergy);

char *strtokm(char *str, const char *delim);

//const char pkgDir[] = "/Users/sdenboer/code/scriptie/rapl/intel-rapl:0/";
const char pkgDir[] = "/sys/class/powercap/intel-rapl/intel-rapl:0/";
//const char dramDir[] = "/Users/sdenboer/code/scriptie/rapl/intel-rapl:0/intel-rapl:0:2/";
const char dramDir[] = "/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:2/";
//const char psysDir[] = "/Users/sdenboer/code/scriptie/rapl/intel-rapl:1/";
const char psysDir[] = "/sys/class/powercap/intel-rapl/intel-rapl:1/";
//const char cpuCyclesFile[] = "/Users/sdenboer/code/scriptie/proc/stat";
const char cpuCyclesFile[] = "/proc/stat";
//const char cpuCyclesPidFile[] = "/Users/sdenboer/code/scriptie/proc/[pid]/stat";
const char cpuCyclesPidFile[] = "/proc/[pid]/stat";
//const char command[] = "ps -o pid= | xargs | tr -d '\n'";
//const char command[] = "ps -efho pid -t '?' | xargs | tr -d '\n'";
//const char command[] = "ps -efh -o \"%p|ENERGY_METER_DELIMITER|\" -o command -t '?' | more | awk '{$1=$1;print}'";
const char command[] = "ps -a -o pid -o args | tail -n +2 | awk '{$1=$1;print}' | sed -r 's/\\s+/|ENERGY_METER_DELIMITER|/'";

bool pkgSupported;
bool dramSupported;
bool psysSupported;


int main(int argc, char **argv) {

    signal(SIGINT, handle_int);
    signal(SIGTERM, handle_int);

    while (!stop) {
        printf("Starting Energy Meter\n");
        long cpuCycleBusy_1;
        long cpuCycleTotal_1;
        float cpuEnergy_1;

        struct PidInfo *pidInfoList;

        int pidAmount;

        long cpuCycleBusy_2;
        long cpuCycleTotal_2;
        float cpuEnergy_2;
        pkgSupported = checkRAPLSupport(pkgDir, "package-0");
        dramSupported = checkRAPLSupport(dramDir, "package-0");
        psysSupported = checkRAPLSupport(psysDir, "psys");

        if (!pkgSupported && !dramSupported && !psysSupported) {
            printf("Custom energy model not supported yet\n");
            exit(1);
        }
        FILE *fpt;

        fpt = fopen("EnergyReadings.csv", "w+");
        fprintf(fpt, "PID, Joules, Utilization\n");
        fflush(fpt);

        while (!stop) {

            calculateCpuCycles(&cpuCycleBusy_1, &cpuCycleTotal_1);

            setPidArray(&pidInfoList, &pidAmount);

            calculateEnergy(&cpuEnergy_1);

            sleep(1);

            calculateCpuCycles(&cpuCycleBusy_2, &cpuCycleTotal_2);
            updatePidArray(&pidInfoList, &pidAmount);


            calculateEnergy(&cpuEnergy_2);

            float total = (float) cpuCycleTotal_2 - (float) cpuCycleTotal_1;
            float busy = (float) cpuCycleBusy_2 - (float) cpuCycleBusy_1;
            float cpuUtil = total != 0 ? busy / total : 0;
            float cpuPower = cpuEnergy_2 - cpuEnergy_1;

            for (int i = 0; i < pidAmount; ++i) {
                char *pid = (char *) &pidInfoList[i].pid;
                long *tot_2 = &pidInfoList[i].total_2;
                long *tot_1 = &pidInfoList[i].total_1;
                char *command = &pidInfoList[i].command;
                long timed = (*tot_2 - *tot_1);
                float cpuPidUtil = total != 0 ? (float) timed / total : 0;
                float cpuPidPower = cpuUtil != 0 ? ((cpuPidUtil * cpuPower) / cpuUtil) : 0;
                if (cpuPidPower > 0) {
                    printf("%.2f Watt voor PID %s\n", cpuPidPower, pid);
                    fprintf(fpt, "%s, %.5f, %.5f\n", pid, cpuPidPower, cpuPidUtil);
                    fflush(fpt);
                }

            }
//            printf("%.2f Watt voor CPU\n", cpuPower);
//            fprintf(fpt, "%s, %.5f, %.5f\n", "CPU", cpuPower, cpuUtil);

            fflush(stdout);

        }
        fclose(fpt);
    }
    printf("Exiting Energy Meter");
    return 0;

}

void setPidArray(struct PidInfo *list[], int *pidAmount) {
    FILE *fp;
    int i, count = 0;
    fp = popen(command, "r"); //MAC
    char buffer[10000];
    *list = malloc(1 * sizeof(struct PidInfo));
    int j = 0;
    while (fgets(buffer, 10000, fp)) {
        count++;
        *list = realloc(*list, count * sizeof(struct PidInfo));
        char *token = strtokm(buffer, "|ENERGY_METER_DELIMITER|");
        bool pidToken = true;
        while (token != NULL) {
            if (pidToken) {
                strncpy((*list)[count - 1].pid, token, 10);
                (*list)[count - 1].total_1 = calculateCpuCyclesPid(token);
                pidToken = false;
            } else {
                strncpy((*list)[count - 1].command, str_replace(token, "\n", ""), 10000);
            }
            token = strtokm(NULL, "|ENERGY_METER_DELIMITER|");
        }
        j++;
    }
    *pidAmount = count;

    fclose(fp);


}

char *strtokm(char *str, const char *delim) {
    static char *tok;
    static char *next;
    char *m;

    if (delim == NULL) return NULL;

    tok = (str) ? str : next;
    if (tok == NULL) return NULL;

    m = strstr(tok, delim);

    if (m) {
        next = m + strlen(delim);
        *m = '\0';
    } else {
        next = NULL;
    }

    return tok;
}

void updatePidArray(struct PidInfo *list[], int *pidAmount) {
    for (int k = 0; k < *pidAmount; ++k) {
        (*list)[k].total_2 = calculateCpuCyclesPid((*list)[k].pid);
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

void calculateEnergy(float *cpuEnergy) {
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
    float j = (microjoules / 1000000.0);
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
    char *line = malloc(bufferLength * sizeof(char));
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
    char *result;
    char *ins;
    char *tmp;
    int len_rep;
    int len_with;
    int len_front;
    int count;

    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL;
    if (!with)
        with = "";
    len_with = strlen(with);


    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}
