using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using System.Data.Sql;
using System.Data.SqlClient;
using System.Data.SqlTypes;
using System.Web.Security;
using KarottoXros.Code;
namespace KarottoXros.Controllers
{
    public class UserController : Controller
    {
        public ActionResult Login()
        {
            return View();
        }

        public void LoginAction()
        {
            string strname = Request.Form["TextName"];
            string strpwd = Request.Form["TextPwd"];
            if (strname != null && strpwd != null)
            {
                
                Models.UserManage usermanage = new Models.UserManage();
                string role = usermanage.Login(strname, strpwd);
                Response.Write("<p>"+role+"</p>");
                if (role == null)
                {
                    Response.Write("<script>alert('密码和账号不符');</script>");
                }
                else
                {

                    FormsAuthentication.SetAuthCookie(strname, false);
                    Response.Write("<script>alert('登陆成功');</script>");
                }
            }
            else
            {
                Response.Write("<script>alert('用户密码其中一个都不能为空');</script>");
            }
            
        }

        [KXros(Roles = "admin")]
        public ActionResult IsOk()
        {

            return View();
        }

        public ActionResult Register()
        {
            return View();
        }

        public void RegisterAction()
        {
            string strname = Request.Form["TextName"];
            string strpwd = Request.Form["TextPwd"];
            Models.UserModels user = new Models.UserModels();
            user.UserName = strname;
            user.UserPassword = strpwd;
            user.UserRole = "no";
            Models.UserManage usermanage = new Models.UserManage();
            string o = usermanage.insert(user);
            Response.Write(o);
            
        }

        public ActionResult Delete()
        {
            return View();
        }

        public void DeleteAction()
        {
            string strname = Request.Form["uID"];
            Models.UserModels user = new Models.UserModels();
            user.UserName = strname;
            user.UserPassword = null;
            user.UserRole = null;
            Models.UserManage usermanage = new Models.UserManage();
            string o = usermanage.delete(user);
            Response.Write(o);
        }

        public ActionResult AlterInfo()
        {
            string user_name = Request.QueryString["UserName"];
            ViewData["UserName"] = user_name;
            return View();
        }

        public void AlterInfoAction()
        {
            string user_name = Request.Form["TextName"];
            string user_pwd = Request.Form["TextPwd"];
            string user_pwd2 = Request.Form["TextPwd2"];
            Models.UserManage usermanage = new Models.UserManage();
            string role = usermanage.Login(user_name, user_pwd);
            if (role == null)
            {
                Response.Write("<script>alert('密码不对');</script>");
            }
            else
            {
                Models.UserModels user = new Models.UserModels();
                user.UserName = user_name;
                user.UserPassword = user_pwd2;
                string o = usermanage.alter(user_name, user);
                Response.Write(o);
            }
        }

    }
}
