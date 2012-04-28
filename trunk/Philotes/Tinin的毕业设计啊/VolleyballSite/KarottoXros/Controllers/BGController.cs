using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace KarottoXros.Controllers
{
    public class BGController : Controller
    {
        //
        // GET: /BG/

        public ActionResult Index()
        {
            try
            {
                Models.IUserManage usermanage = new Models.UserManage();
                List<Models.UserModels> user_list;
                user_list = usermanage.select("");
                ViewData["UserList"] = user_list;
                return View();
            }
            catch (Exception ex)
            {
                Response.Write(ex.Message);
                return View();
            }
            
        }

    }
}
