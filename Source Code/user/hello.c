#include "sdk.h"
static UiScene scene;
int main(void){int w=Window("Hello / 已安装的原生应用",530,280);if(w<0)return 1;ui_begin(&scene);ui_text(&scene,24,62,"你好，这是通过独立安装器安装的应用。");ui_text(&scene,24,104,"应用文件来自原生 SXP 包，运行于 Ring 3。");ui_text(&scene,24,144,"可在应用管理器中修复、升级或卸载。");ui_button(&scene,24,204,140,"关闭");Scene(w,&scene);for(;;){UiEvent e;if(Event(&e)==0&&(ui_hit(&e,24,204,140,26)||(e.type==MSG_KEY&&e.key==27)))return 0;Yield(1);}}
