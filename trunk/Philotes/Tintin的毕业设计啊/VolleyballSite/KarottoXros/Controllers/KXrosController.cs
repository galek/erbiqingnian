using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using System.Web.Security;
using KarottoXros.Code;
namespace KarottoXros.Controllers
{
    public class KXrosController : Controller
    {
        //
        // GET: /KXros/
        [MyAuth]
        public ActionResult Index()
        {
            return View();
        }

        public ActionResult Login()
        {
            //FormsAuthentication.SetAuthCookie("aaa", false);   
            return View();
        }

    }
}
