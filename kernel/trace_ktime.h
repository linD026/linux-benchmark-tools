#ifndef __TRACE_KTIME_H__
#define __TRACE_KTIME_H__

#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/overflow.h>

struct trace_ktime_node {
	ktime_t start;
	ktime_t end;
	struct list_head node;
};

struct trace_ktime {
	const char *prefix;
	struct list_head head;
	spinlock_t lock;
};

#define DEFINE_TRACE_KTIME(_name, _prefix)          \
	struct trace_ktime _name = {                \
		.prefix = _prefix,                  \
		.head = LIST_HEAD_INIT(_name.head),       \
		.lock = __SPIN_LOCK_UNLOCKED(lock), \
	}

static inline struct trace_ktime_node *trace_ktime_get(struct trace_ktime *tk)
{
	struct trace_ktime_node *tkn =
		kmalloc(sizeof(struct trace_ktime_node), GFP_KERNEL);

	if (unlikely(!tkn))
		return NULL;

	spin_lock(&tk->lock);
	list_add_tail(&tkn->node, &tk->head);
	spin_unlock(&tk->lock);

	return tkn;
}

#define DEFINE_TRACE_KTIME_NODE(_name, trace_ktime) \
	struct trace_ktime_node *_name = trace_ktime_get(trace_ktime)

#define trace_ktime_get_start(_name) (_name)->start = ktime_get()

#define trace_ktime_get_end(_name) (_name)->end = ktime_get()

static inline void print_trace_ktime(struct trace_ktime *tk)
{
	s64 total_during = 0;
	unsigned long cnt = 0;
	struct trace_ktime_node *pos, *n;

	list_for_each_entry_safe(pos, n, &tk->head, node) {
		s64 tmp = 0;
		s64 during = ktime_to_us(ktime_sub(pos->end, pos->start));

		if (check_add_overflow(total_during, during, &tmp)) {
			pr_info("[trace ktime] %s: flush during due to overflow at %lu\n",
				tk->prefix, cnt);
			pr_info("[trace ktime] %s: total %lld us with %lu time(s), avg %lld us\n",
				tk->prefix, total_during, cnt,
				total_during / cnt);
			cnt = 0;
			total_during = during;
		} else
			total_during = tmp;
		cnt++;
		list_del_init(&pos->node);
		kfree(pos);
	}

	pr_info("[trace ktime] %s: total %lld us with %lu time(s), avg %lld us\n",
		tk->prefix, total_during, cnt, total_during / cnt);
}

#endif /* __TRACE_KTIME_H__ */
