#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	printf ("system call!\n");
	//Linux에서 시스템 콜의 번호는 rax에 저장됨
	uint64_t number = f -> R.rax;
	//나머지 인자의 레지스터 순서 : %rdi, %rsi, %rdx, %rcx, %r8, %r9
	//참고 : https://rninche01.tistory.com/entry/Linux-system-call-table-%EC%A0%95%EB%A6%ACx86-x64
	//리턴값이 있는 시스템 콜의 경우, 리턴 값을 %rax(32비트면 %eax)에 저장해야 한다
	switch(number)
	{
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			//void exit(int status)
			exit(f -> R.rdi);
			break;
		case SYS_EXEC:
			//int exec(const char *file)
			f -> R.rax = exec(f -> R.rdi);
			break;
		case SYS_WAIT:
			//int wait(pid_t)
			f -> R.rax = wait(f -> R.rdi);
			break;
		case SYS_FORK:
			//pid_t fork(const char *thread_name)
			f -> R.rax = fork(f -> R.rdi);
		default:
			exit(-1);
	}

	thread_exit ();
}

//유저가 건네준 메모리 주소가 유효한지를 판별하는 함수 check_address
void check_address(void * address)
{
	//현재 스레드에서 pml4를 받아오기 위해 현재 스레드 정보 받아오기
	//pml4 : Page Map Level 4, x86-64 아키텍처에서 가상 주소를 물리 주소로 변환할때 사용함
	struct thread * current_t = thread_current();

	//건네받은 주소가 커널 영역의 주소거나, NULL이거나, 현재 스레드의 페이지 맵 레벨 4 테이블에 주소가 없는 경우
	if (is_kernel_vaddr(address) || address == NULL || pml4_get_page(current_t -> pml4, address) == NULL)
	{	
		//실행 종료
		exit(-1);
	}
}

//void halt(void) NO_RETURN
void halt(void)
{
	power_off();
}

//void exit(int status) NO_RETURN
void exit(int status)
{
	//프로세스 이름 : exit(status)가 출력되어야 함
	printf("%s: exit(%d)\n", thread_current() -> name, thread_current() -> status);

	thread_exit();
}

//int exec(const char *cmd_line)
int exec(const char *cmd_line)
{

}

//int wait(pid_t)
int wait(int pid_t)
{

}

//pid_t fork(const char *thread_name)
int fork(const char *thread_name)
{
	check_address(thread_name);

	return process_fork(thread_current(), NULL);
}