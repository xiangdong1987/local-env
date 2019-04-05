#pragma once

#include "coroutine.h"
#include "socket.h"
#include "zend_vm.h"
#include "zend_closures.h"

#include <stack>

#define SW_DEFAULT_MAX_CORO_NUM              3000
#define SW_DEFAULT_PHP_STACK_PAGE_SIZE       8192

#define SWOG ((zend_output_globals *) &OG(handlers))

typedef enum
{
    SW_CORO_CONTEXT_RUNNING,
    SW_CORO_CONTEXT_IN_DELAYED_TIMEOUT_LIST,
    SW_CORO_CONTEXT_TERM
} php_context_state;

enum sw_coro_hook_type
{
    SW_HOOK_FILE = 1u << 1,
    SW_HOOK_SLEEP = 1u << 2,
    SW_HOOK_TCP = 1u << 3,
    SW_HOOK_UDP = 1u << 4,
    SW_HOOK_UNIX = 1u << 5,
    SW_HOOK_UDG = 1u << 6,
    SW_HOOK_SSL = 1u << 7,
    SW_HOOK_TLS = 1u << 8,
    SW_HOOK_BLOCKING_FUNCTION = 1u << 9,
    SW_HOOK_ALL = 0x7fffffff,
};

struct php_coro_task
{
    JMP_BUF *bailout;
    zval *vm_stack_top;
    zval *vm_stack_end;
    zend_vm_stack vm_stack;
    size_t vm_stack_page_size;
    zend_execute_data *execute_data;
    zend_error_handling_t error_handling;
    zend_class_entry *exception_class;
    zend_object *exception;
    zend_output_globals *output_ptr;
    SW_DECLARE_EG_SCOPE(scope);
    swoole::Coroutine *co;
    std::stack<php_swoole_fci *> *defer_tasks;
    long pcid;
    zend_object *context;
};

struct php_coro_args
{
    zend_fcall_info_cache *fci_cache;
    zval *argv;
    uint32_t argc;
};

// TODO: remove php coro context
struct php_coro_context
{
    php_context_state state;
    zval coro_params;
    zval *current_coro_return_value_ptr;
    void *private_data;
    swTimer_node *timer;
    php_coro_task *current_task;
};

namespace swoole
{
class PHPCoroutine
{
public:
    static long create(zend_fcall_info_cache *fci_cache, uint32_t argc, zval *argv);
    static void defer(php_swoole_fci *fci);

    static void check_bind(const char *name, long bind_cid);

    static bool enable_hook(int flags);
    static bool disable_hook();

    // TODO: remove old coro APIs (Manual)
    static void yield_m(zval *return_value, php_coro_context *sw_php_context);
    static int resume_m(php_coro_context *sw_current_context, zval *retval, zval *coro_retval);

    static inline void init()
    {
        Coroutine::set_on_yield(on_yield);
        Coroutine::set_on_resume(on_resume);
        Coroutine::set_on_close(on_close);
    }

    static inline long get_cid()
    {
        return likely(active) ? Coroutine::get_current_cid() : -1;
    }

    static inline long get_pcid()
    {
        php_coro_task *task = (php_coro_task *) Coroutine::get_current_task();
        return likely(task) ? task->pcid : -1;
    }

    static inline php_coro_task* get_task()
    {
        php_coro_task *task = (php_coro_task *) Coroutine::get_current_task();
        return task ? task : &main_task;
    }

    static inline php_coro_task* get_origin_task(php_coro_task *task)
    {
        Coroutine *co = task->co->get_origin();
        return co ? (php_coro_task *) co->get_task() : &main_task;
    }

    static inline php_coro_task* get_task_by_cid(long cid)
    {
        return cid == -1 ? &main_task : (php_coro_task *) Coroutine::get_task_by_cid(cid);
    }

    static inline uint64_t get_max_num()
    {
        return max_num;
    }

    static inline void set_max_num(uint64_t n)
    {
        max_num = n;
    }

protected:
    static bool active;
    static uint64_t max_num;
    static php_coro_task main_task;

    static inline void vm_stack_init(void);
    static inline void vm_stack_destroy(void);
    static inline void save_vm_stack(php_coro_task *task);
    static inline void restore_vm_stack(php_coro_task *task);
    static inline void save_og(php_coro_task *task);
    static inline void restore_og(php_coro_task *task);
    static inline void save_task(php_coro_task *task);
    static inline void restore_task(php_coro_task *task);
    static void on_yield(void *arg);
    static void on_resume(void *arg);
    static void on_close(void *arg);
    static void create_func(void *arg);
};
}

