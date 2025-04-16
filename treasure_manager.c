#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

typedef struct {
    int treasure_id;
    char user_name[50];
    double latitude;
    double longitude;
    char clue_text[256];
    int value;
} Treasure;


void add_treasure(const char *hunt_id);
void list_treasures(const char *hunt_id);
void view_treasure(const char *hunt_id, const char *treasure_id);
void remove_treasure(const char *hunt_id, const char *treasure_id);
void remove_hunt(const char *hunt_id);
void log_operation(const char *hunt_id, const char *operation);
void create_symbolic_link(const char *hunt_id);
void error_exit(const char *message);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: treasure_manager --operation [hunt_id] [treasure_id]\n");
        return 1;
    }

    
    if (strcmp(argv[1], "--add") == 0) {
        if (argc != 3) {
            printf("Usage: treasure_manager --add [hunt_id]\n");
            return 1;
        }
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        if (argc != 3) {
            printf("Usage: treasure_manager --list [hunt_id]\n");
            return 1;
        }
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0) {
        if (argc != 4) {
            printf("Usage: treasure_manager --view [hunt_id] [treasure_id]\n");
            return 1;
        }
        view_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove_treasure") == 0) {
        if (argc != 4) {
            printf("Usage: treasure_manager --remove_treasure [hunt_id] [treasure_id]\n");
            return 1;
        }
        remove_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        if (argc != 3) {
            printf("Usage: treasure_manager --remove_hunt [hunt_id]\n");
            return 1;
        }
        remove_hunt(argv[2]);
    } else {
        printf("Unknown operation: %s\n", argv[1]);
        return 1;
    }

    return 0;
}


void error_exit(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}


void ensure_hunt_directory(const char *hunt_id) {
    struct stat st = {0};
    
    if (stat(hunt_id, &st) == -1) {
        if (mkdir(hunt_id, 0700) == -1) {
            error_exit("Failed to create hunt directory");
        }
    }
}


void log_operation(const char *hunt_id, const char *operation) {
    char log_path[512];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);
    
    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd == -1) {
        error_exit("Failed to open log file");
    }
    
    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", timestamp, operation);
    
    write(fd, log_entry, strlen(log_entry));
    close(fd);
    
    
    create_symbolic_link(hunt_id);
}


void create_symbolic_link(const char *hunt_id) {
    char log_path[512];
    char link_path[512];
    
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);
    snprintf(link_path, sizeof(link_path), "logged_hunt-%s", hunt_id);
    
   
    unlink(link_path);
    
    
    if (symlink(log_path, link_path) == -1) {
        error_exit("Failed to create symbolic link");
    }
}



void add_treasure(const char *hunt_id) {
    char treasures_path[512];
    Treasure new_treasure;
    char operation[512];
    
    ensure_hunt_directory(hunt_id);
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures", hunt_id);
    
   
    printf("Enter treasure ID (integer): ");
    scanf("%d", &new_treasure.treasure_id);
    
    printf("Enter username: ");
    scanf("%s", new_treasure.user_name);
    
    printf("Enter latitude: ");
    scanf("%lf", &new_treasure.latitude);
    
    printf("Enter longitude: ");
    scanf("%lf", &new_treasure.longitude);
    
    printf("Enter clue text: ");
    getchar(); 
    fgets(new_treasure.clue_text, sizeof(new_treasure.clue_text), stdin);
    
    new_treasure.clue_text[strcspn(new_treasure.clue_text, "\n")] = 0;
    
    printf("Enter value (integer): ");
    scanf("%d", &new_treasure.value);
    
    
    int fd = open(treasures_path, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd == -1) {
        error_exit("Failed to open treasures file");
    }
    
    
    if (write(fd, &new_treasure, sizeof(Treasure)) != sizeof(Treasure)) {
        close(fd);
        error_exit("Failed to write treasure to file");
    }
    
    close(fd);
    
    
    snprintf(operation, sizeof(operation), "Added treasure %d by %s", 
            new_treasure.treasure_id, new_treasure.user_name);
    log_operation(hunt_id, operation);
    
    printf("Treasure added successfully!\n");
}


void list_treasures(const char *hunt_id) {
    char treasures_path[512];
    Treasure treasure;
    struct stat st;
    int fd, read_size;
    char operation[512];
    
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures", hunt_id);
    
   
    if (stat(treasures_path, &st) == -1) {
        printf("Hunt %s does not exist or has no treasures.\n", hunt_id);
        return;
    }
    
    
    fd = open(treasures_path, O_RDONLY);
    if (fd == -1) {
        error_exit("Failed to open treasures file");
    }
    
    
    if (fstat(fd, &st) == -1) {
        close(fd);
        error_exit("Failed to get file information");
    }
    
    
    
    printf("Hunt: %s\n", hunt_id);
    printf("Treasures file size: %ld bytes\n", st.st_size);
    
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
    printf("Last modified: %s\n\n", time_str);
    
    
    printf("Treasures:\n");
    printf("ID\tUser\t\tCoordinates\t\tValue\tClue\n");
    printf("------------------------------------------------------------------\n");
    
    while ((read_size = read(fd, &treasure, sizeof(Treasure))) == sizeof(Treasure)) {
        printf("%d\t%-10s\t(%.6f, %.6f)\t%d\t%s\n", 
               treasure.treasure_id, 
               treasure.user_name, 
               treasure.latitude, 
               treasure.longitude, 
               treasure.value, 
               treasure.clue_text);
    }
    
    close(fd);
    
    
    snprintf(operation, sizeof(operation), "Listed treasures for hunt %s", hunt_id);
    log_operation(hunt_id, operation);
}


void view_treasure(const char *hunt_id, const char *treasure_id_str) {
    char treasures_path[512];
    Treasure treasure;
    int fd, read_size;
    char operation[512];
    int target_id = atoi(treasure_id_str);
    int found = 0;
    
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures", hunt_id);
    
    fd = open(treasures_path, O_RDONLY);
    if (fd == -1) {
        error_exit("Failed to open treasures file");
    }
    
    
    while ((read_size = read(fd, &treasure, sizeof(Treasure))) == sizeof(Treasure)) {
        if (treasure.treasure_id == target_id) {
            found = 1;
            break;
        }
    }
    
    close(fd);
    
    if (found) {
        printf("Treasure ID: %d\n", treasure.treasure_id);
        printf("User: %s\n", treasure.user_name);
        printf("Coordinates: (%.6f, %.6f)\n", treasure.latitude, treasure.longitude);
        printf("Value: %d\n", treasure.value);
        printf("Clue: %s\n", treasure.clue_text);
        
        
        snprintf(operation, sizeof(operation), "Viewed treasure %d in hunt %s", target_id, hunt_id);
        log_operation(hunt_id, operation);
    } else {
        printf("Treasure %s not found in hunt %s.\n", treasure_id_str, hunt_id);
    }
}


void remove_treasure(const char *hunt_id, const char *treasure_id_str) {
    char treasures_path[512];
    char temp_path[512];
    Treasure treasure;
    int fd_src, fd_dest, read_size;
    char operation[512];
    int target_id = atoi(treasure_id_str);
    int found = 0;
    
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures", hunt_id);
    snprintf(temp_path, sizeof(temp_path), "%s/treasures.tmp", hunt_id);
    
    
    fd_src = open(treasures_path, O_RDONLY);
    if (fd_src == -1) {
        error_exit("Failed to open treasures file");
    }
    
   
    fd_dest = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd_dest == -1) {
        close(fd_src);
        error_exit("Failed to create temporary file");
    }
    
    
    while ((read_size = read(fd_src, &treasure, sizeof(Treasure))) == sizeof(Treasure)) {
        if (treasure.treasure_id == target_id) {
            found = 1;
        } else {
            if (write(fd_dest, &treasure, sizeof(Treasure)) != sizeof(Treasure)) {
                close(fd_src);
                close(fd_dest);
                error_exit("Failed to write to temporary file");
            }
        }
    }
    
    close(fd_src);
    close(fd_dest);
    
    if (found) {
        
        if (rename(temp_path, treasures_path) == -1) {
            error_exit("Failed to replace treasures file");
        }
        
        
        snprintf(operation, sizeof(operation), "Removed treasure %d from hunt %s", target_id, hunt_id);
        log_operation(hunt_id, operation);
        
        printf("Treasure %s removed successfully!\n", treasure_id_str);
    } else {
        unlink(temp_path);  
        printf("Treasure %s not found in hunt %s.\n", treasure_id_str, hunt_id);
    }
}


void remove_hunt(const char *hunt_id) {
    char treasures_path[512];
    char log_path[512];
    char link_path[512];
    char operation[512];
    
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures", hunt_id);
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);
    snprintf(link_path, sizeof(link_path), "logged_hunt-%s", hunt_id);
    

    snprintf(operation, sizeof(operation), "Removing hunt %s", hunt_id);
    log_operation(hunt_id, operation);
    
   
    unlink(treasures_path);
    unlink(log_path);
    unlink(link_path);
    
    
    if (rmdir(hunt_id) == -1) {
        error_exit("Failed to remove hunt directory");
    }
    
    printf("Hunt %s removed successfully!\n", hunt_id);
}
