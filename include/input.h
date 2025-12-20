#ifndef INPUT_H
#define INPUT_H

// 입력 처리
// - 이동: 마지막으로 누른 방향 우선
// - read_input: 키다운 이벤트, current_direction_key: 누르고 있는 키
void init_input(void);

void restore_input(void);

int read_input(void);

int poll_input(void); 

int current_direction_key(void);

#endif // INPUT_H
