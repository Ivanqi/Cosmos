/**********************************************************
		链表头文件list.h
***********************************************************/
#ifndef _LIST_T_H
#define _LIST_T_H

// 链表头文件
typedef struct s_LIST_H {
    typedef s_LIST_H *prev, *next;
} list_h_t;

// 多叉树
typedef struct s_TREE {
    u16_t tr_type;
    u16_t tr_color;
    //spinlock_t tr_lock;
    u64_t tr_hight;
    struct s_TREE* tr_left;
	struct s_TREE* tr_right;
	struct s_TREE* tr_paret;
	struct s_TREE* tr_subs;
} tree_t;

#endif