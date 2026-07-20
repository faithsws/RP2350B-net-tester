#ifndef NET_TESTER_NETDBG_H
#define NET_TESTER_NETDBG_H

#include <lvgl.h>
#include <rtthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 创建调试二级菜单（网络协议）屏幕 */
lv_obj_t * net_tester_netdbg_create_menu(void);

/* 绑定当前可见页的焦点组；返回菜单屏或功能页 */
void net_tester_netdbg_bind_group(lv_group_t * group);

/* ESC：功能页→菜单 返回 true；已在菜单则返回 false（由上层回主屏） */
bool net_tester_netdbg_handle_esc(void);

/* 当前是否在网络调试相关屏幕（菜单或功能页） */
bool net_tester_netdbg_is_on_screen(void);

/* Tab 键重映射（IP/ASCII 编辑态） */
uint32_t net_tester_netdbg_key_remap(uint32_t key);

/* 离开调试时清理编辑态/后台任务 */
void net_tester_netdbg_on_leave(void);

/* 二级菜单 ESC 回主屏时回调（由 UI 层注册 back_to_main） */
typedef void (*net_tester_netdbg_leave_cb_t)(void);
void net_tester_netdbg_set_leave_cb(net_tester_netdbg_leave_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_NETDBG_H */
