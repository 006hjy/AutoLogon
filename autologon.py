import tkinter as tk
from tkinter import ttk, messagebox
import winreg
import ctypes


def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False


class AutoLoginConfigurator:
    def __init__(self, master):
        self.master = master
        master.title("Windows自动登录配置工具 v1.0")
        master.geometry("400x220")

        # 注册表路径定义
        self.reg_path = r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
        self.keys = {
            'auto_logon': 'AutoAdminLogon',
            'username': 'DefaultUserName',
            'password': 'DefaultPassword'
        }

        # 初始化界面
        self.create_widgets()
        self.load_registry_values()

    def create_widgets(self):
        # 复选框组件
        self.auto_login_var = tk.IntVar()
        chk_auto = ttk.Checkbutton(
            self.master,
            text="启用自动登录",
            variable=self.auto_login_var
        )
        chk_auto.grid(row=0, column=0, padx=10, pady=10, sticky='w')

        # 用户名输入框
        lbl_user = ttk.Label(self.master, text="用户名:")
        lbl_user.grid(row=1, column=0, padx=10, sticky='w')
        self.entry_user = ttk.Entry(self.master, width=30)
        self.entry_user.grid(row=1, column=1, padx=10, sticky='ew')

        # 密码输入框
        lbl_pwd = ttk.Label(self.master, text="密码:")
        lbl_pwd.grid(row=2, column=0, padx=10, sticky='w')
        self.entry_pwd = ttk.Entry(self.master, width=30)
        self.entry_pwd.grid(row=2, column=1, padx=10, sticky='ew')

        # 保存按钮
        btn_save = ttk.Button(self.master, text="保存配置", command=self.save_config)
        btn_save.grid(row=3, column=1, pady=20, sticky='e')

        # 网格列配置
        self.master.columnconfigure(1, weight=1)

    def load_registry_values(self):
        try:
            reg_key = winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                self.reg_path,
                0,
                winreg.KEY_READ
            )

            # 读取自动登录状态
            try:
                auto_logon, _ = winreg.QueryValueEx(reg_key, self.keys['auto_logon'])
                self.auto_login_var.set(1 if auto_logon == '1' else 0)
            except FileNotFoundError:
                self.auto_login_var.set(0)

            # 读取用户名
            try:
                username, _ = winreg.QueryValueEx(reg_key, self.keys['username'])
                self.entry_user.insert(0, username)
            except FileNotFoundError:
                pass

            # 读取密码
            try:
                password, _ = winreg.QueryValueEx(reg_key, self.keys['password'])
                self.entry_pwd.insert(0, password)
            except FileNotFoundError:
                pass

            winreg.CloseKey(reg_key)
        except Exception as e:
            messagebox.showerror("错误", f"注册表读取失败:\n{str(e)}")

    def save_config(self):
        if not is_admin():
            messagebox.showerror("权限错误", "请以管理员身份运行本程序")
            return

        try:
            reg_key = winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                self.reg_path,
                0,
                winreg.KEY_WRITE
            )
        except FileNotFoundError:
            reg_key = winreg.CreateKey(winreg.HKEY_LOCAL_MACHINE, self.reg_path)

        try:
            # 写入自动登录状态
            auto_logon_value = '1' if self.auto_login_var.get() else '0'
            winreg.SetValueEx(
                reg_key,
                self.keys['auto_logon'],
                0,
                winreg.REG_SZ,
                auto_logon_value
            )

            # 写入用户名
            username = self.entry_user.get()
            winreg.SetValueEx(
                reg_key,
                self.keys['username'],
                0,
                winreg.REG_SZ,
                username
            )

            # 写入密码
            password = self.entry_pwd.get()
            winreg.SetValueEx(
                reg_key,
                self.keys['password'],
                0,
                winreg.REG_SZ,
                password
            )

            messagebox.showinfo("成功", "配置已保存，重启后生效")
        except Exception as e:
            messagebox.showerror("错误", f"注册表写入失败:\n{str(e)}")
        finally:
            winreg.CloseKey(reg_key)


if __name__ == "__main__":
    if not is_admin():
        ctypes.windll.shell32.ShellExecuteW(
            None, "runas", "python.exe", __file__, None, 1)
    else:
        root = tk.Tk()
        app = AutoLoginConfigurator(root)
        root.mainloop()
