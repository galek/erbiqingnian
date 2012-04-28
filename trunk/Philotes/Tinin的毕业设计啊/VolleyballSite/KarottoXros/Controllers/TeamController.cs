using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace KarottoXros.Controllers
{
    public class TeamController : Controller
    {
        public ActionResult Index()
        {
            return View();
        }

        public ActionResult Add()
        {
            Models.TeamManage teammanage = new Models.TeamManage();
            List<string> list_team = teammanage.selectlocation();
            ViewData["location"] = new SelectList(list_team);
            return View();
        }

        public void AddAction()
        {
            string str_name = Request.Form["Name"];
            string str_city = Request.Form["City"];
            int cid = getLocationId(Request.Form["Location"]);
            string info = Request.Form["Info"];
            Models.TeamModels team = new Models.TeamModels();
            Models.ITeamManage teamManage = new Models.TeamManage();
            team.Name = str_name;
            team.City = str_city;
            team.location = cid;
            team.Info = info;
            string o = teamManage.insert(team);
            Response.Write(o);
        }

        private int getLocationId(string location)
        {
            if (location.Equals("东部"))
                return 1;
            else
                return 2;
        }

        public ActionResult List()
        {
            try
            {
                Models.ITeamManage teamManage = new Models.TeamManage();
                List<Models.TeamModels> team_list;
                team_list = teamManage.select("");
                ViewData["TeamList"] = team_list;
                return View();
            }
            catch (Exception ex)
            {
                Response.Write(ex.Message);
                return View();
            }
        }

        public ActionResult Show()
        {

            try
            {
                Models.ITeamManage teammanage = new Models.TeamManage();
                int tid = Convert.ToInt32(Request.QueryString["TId"]);
                Models.TeamModels team = teammanage.selectById(tid);
                ViewData["Tid"] = tid;
                ViewData["Name"] = team.Name;
                ViewData["City"] = team.City;
                ViewData["Info"] = team.Info;
                ViewData["Location"] = getLocation(team.location);
                return View();
            }
            catch (Exception ex)
            {
                Response.Write(ex.Message);
                return View();
            }
        }

        private string getLocation(int lid)
        {
            if (lid == 1)
                return "东部";
            else
                return "西部";
        }

        public ActionResult Alter()
        {
            
            try
            {
                Models.ITeamManage teammanage = new Models.TeamManage();
                int tid = Convert.ToInt32(Request.QueryString["TId"]);
                Models.TeamModels team = teammanage.selectById(tid);
                ViewData["Tid"] = tid;
                ViewData["Name"] = team.Name;
                ViewData["City"] = team.City;
                ViewData["Info"] = team.Info;
                List<string> list_team = new List<string>();
                list_team.Add("东部");
                list_team.Add("西部");
                ViewData["Location"] = new SelectList(list_team);
                return View();
            }
            catch (Exception ex)
            {
                Response.Write(String.Format("<script>alert('{0}');</script>", ex.Message));
                return View();
            }
        }

        public void AlterAction()
        {
            try
            {
                Models.TeamManage teammanage = new Models.TeamManage();
                Models.TeamModels team = new Models.TeamModels();
                int tid = Convert.ToInt32(Request.Form["Tid"]);
                team.Name = Request.Form["Name"];
                team.City = Request.Form["City"];
                team.Info = Request.Form["Info"];
                team.location = getLocationId(Request.Form["location"]);
                string o = teammanage.alter(tid, team);
                Response.Write(o);
            }
            catch (Exception ex)
            {
                Response.Write(String.Format("<script>alert('{0}');</script>",ex.Message));
            }
            
        }

    }
}
