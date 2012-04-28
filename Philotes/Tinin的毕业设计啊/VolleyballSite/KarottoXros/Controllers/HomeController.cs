using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using KarottoXros.Code;
namespace KarottoXros.Controllers
{
    [HandleError]
    public class HomeController : Controller
    {
        public ActionResult Index()
        {
            //ViewData["Date"] = DateTime.Today.Year + "-" + DateTime.Today.Month + "-" + DateTime.Today.Day;
            Session["Role"] = "游客";
            return View();
        }
        [MyAuth(Roles = "Admin")]
        public ActionResult About()
        {
            return View();
        }
    }
}
