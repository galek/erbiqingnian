using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using System.Web.Security;
namespace KarottoXros.Code
{
    public class filter : AuthorizeAttribute
    {
    }


    public class MyAuthAttribute : AuthorizeAttribute
    {
        
        // 只需重载此方法，模拟自定义的角色授权机制   
        protected override bool AuthorizeCore(HttpContextBase httpContext)
        {
            string currentRole = GetRole(httpContext.User.Identity.Name);
            if (Roles.Contains(currentRole))
                return true;
            return base.AuthorizeCore(httpContext);
        }

        // 返回用户对应的角色， 在实际中， 可以从SQL数据库中读取用户的角色信息   
        private string GetRole(string name)
        {
            try
            {
                SqlManager sql = new SqlManager();
                string selectstr = "select UserRole from UserTable where userName = '" + name + "'";
                string strrole = Convert.ToString(sql.ExecuteScalar(selectstr));
                return strrole;
            }
            catch (Exception ex)
            {
                return ("<script>alert('" + ex.Message + "');</script>");
            }
            //return null;
        }
    }   


    public class KXrosAttribute : AuthorizeAttribute
    {
        protected override bool AuthorizeCore(HttpContextBase httpContext)
        {
            string currentRole = GetRole(httpContext.User.Identity.Name);
            if (Roles.Contains(currentRole))
                return true;
            return base.AuthorizeCore(httpContext);   
        }

        public string GetRole(string name)
        {
            SqlManager sql = new SqlManager();
            string getRole = "select UserRole from UserTable where userName = '" + name + "'";
            string role = Convert.ToString(sql.ExecuteScalar(getRole));
            return role;
        }   

    }
}