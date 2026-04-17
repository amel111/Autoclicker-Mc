#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Get Hyprland instance signature dynamically
static void get_hypr_sig(char *buf, size_t len) {
    DIR *d = opendir("/run/user/1000/hypr");
    struct dirent *ent;
    buf[0] = '\0';
    if (!d) return;
    while ((ent = readdir(d))) {
        if (ent->d_name[0] != '.') {
            snprintf(buf, len, "%s", ent->d_name);
            break;
        }
    }
    closedir(d);
}

// Helper to run autoclicker script asynchronously
void run_autoclicker(const char* button, const char* action) {
    pid_t pid = fork();
    if (pid == 0) {
        pid_t grandchild = fork();
        if (grandchild == 0) {
            char hypr_sig[256];
            get_hypr_sig(hypr_sig, sizeof(hypr_sig));
            
            char env_hypr[512], env_xdg[512], env_disp[512], env_dbus[512], env_sock[512];
            snprintf(env_hypr, sizeof(env_hypr), "HYPRLAND_INSTANCE_SIGNATURE=%s", hypr_sig);
            snprintf(env_xdg, sizeof(env_xdg), "XDG_RUNTIME_DIR=/run/user/1000");
            snprintf(env_disp, sizeof(env_disp), "DISPLAY=:0");
            snprintf(env_dbus, sizeof(env_dbus), "DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus");
            snprintf(env_sock, sizeof(env_sock), "YDOTOOL_SOCKET=/tmp/.ydotool_socket");
            
            char* envp[] = { env_hypr, env_xdg, env_disp, env_dbus, env_sock, NULL };
            char script_cmd[512];
            snprintf(script_cmd, sizeof(script_cmd), "/bin/bash /home/azzuhry/.config/scripts/autoclicker.sh %s %s", button, action);
            char* argv2[] = { "su", "azzuhry", "-c", script_cmd, NULL };
            
            execve("/usr/bin/su", argv2, envp);
            exit(1); 
        }
        exit(0);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
}

// Helper to inject a key release (fixes the bug where game ignores ydotool because physical button is held)
void inject_synthetic_release(int btn_code) {
    struct input_event release_ev;
    memset(&release_ev, 0, sizeof(release_ev));
    release_ev.type = EV_KEY;
    release_ev.code = btn_code;
    release_ev.value = 0; // Release
    fwrite(&release_ev, sizeof(release_ev), 1, stdout);
    
    struct input_event syn_ev;
    memset(&syn_ev, 0, sizeof(syn_ev));
    syn_ev.type = EV_SYN;
    syn_ev.code = SYN_REPORT;
    syn_ev.value = 0;
    fwrite(&syn_ev, sizeof(syn_ev), 1, stdout);
    fflush(stdout);
}

int main() {
    struct input_event ev;
    
    // hold trackers
    int forward_held = 0;
    int left_held = 0;
    int right_held = 0;
    
    // active trackers
    int left_autoclicking = 0;
    int right_autoclicking = 0;

    char hypr_sig[256];

    FILE *log = fopen("/tmp/combo_debug.log", "a");

    get_hypr_sig(hypr_sig, sizeof(hypr_sig));
    if (log) {
        fprintf(log, "Starting combo, HYPR_SIG=%s\n", hypr_sig);
        fflush(log);
    }

    while (fread(&ev, sizeof(ev), 1, stdin)) {
        if (ev.type == EV_KEY) {
            
            // --- FORWARD BUTTON ---
            if (ev.code == BTN_EXTRA) {
                forward_held = ev.value;
                
                if (forward_held == 1) {
                    if (left_held && !left_autoclicking) {
                        if (log) { fprintf(log, "COMBO LEFT START (Delayed)\n"); fflush(log); }
                        inject_synthetic_release(BTN_LEFT);
                        run_autoclicker("left", "start");
                        left_autoclicking = 1;
                    }
                    if (right_held && !right_autoclicking) {
                        if (log) { fprintf(log, "COMBO RIGHT START (Delayed)\n"); fflush(log); }
                        inject_synthetic_release(BTN_RIGHT);
                        run_autoclicker("right", "start");
                        right_autoclicking = 1;
                    }
                } else { // forward_held == 0
                    if (left_autoclicking) {
                        if (log) { fprintf(log, "COMBO LEFT STOP (Forward Released)\n"); fflush(log); }
                        run_autoclicker("left", "stop");
                        left_autoclicking = 0;
                    }
                    if (right_autoclicking) {
                        if (log) { fprintf(log, "COMBO RIGHT STOP (Forward Released)\n"); fflush(log); }
                        run_autoclicker("right", "stop");
                        right_autoclicking = 0;
                    }
                }
                
                fwrite(&ev, sizeof(ev), 1, stdout);
                fflush(stdout);
                continue;
            }
            
            // --- LEFT CLICK ---
            if (ev.code == BTN_LEFT) {
                if (ev.value == 1) { 
                    left_held = 1;
                    if (forward_held && !left_autoclicking) {
                        if (log) { fprintf(log, "COMBO LEFT START\n"); fflush(log); }
                        run_autoclicker("left", "start");
                        left_autoclicking = 1;
                        continue; // swallowed
                    }
                } else if (ev.value == 0) { 
                    left_held = 0;
                    if (left_autoclicking) {
                        if (log) { fprintf(log, "COMBO LEFT STOP\n"); fflush(log); }
                        run_autoclicker("left", "stop");
                        left_autoclicking = 0;
                        continue; // swallowed
                    }
                }
            }
            
            // --- RIGHT CLICK ---
            if (ev.code == BTN_RIGHT) {
                if (ev.value == 1) {
                    right_held = 1;
                    if (forward_held && !right_autoclicking) {
                        if (log) { fprintf(log, "COMBO RIGHT START\n"); fflush(log); }
                        run_autoclicker("right", "start");
                        right_autoclicking = 1;
                        continue; // swallowed
                    }
                } else if (ev.value == 0) {
                    right_held = 0;
                    if (right_autoclicking) {
                        if (log) { fprintf(log, "COMBO RIGHT STOP\n"); fflush(log); }
                        run_autoclicker("right", "stop");
                        right_autoclicking = 0;
                        continue; // swallowed
                    }
                }
            }
        }

        fwrite(&ev, sizeof(ev), 1, stdout);
        fflush(stdout);
    }

    if (log) fclose(log);
    return 0;
}
