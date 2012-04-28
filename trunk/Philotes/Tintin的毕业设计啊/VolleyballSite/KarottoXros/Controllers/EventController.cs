using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace KarottoXros.Controllers
{
    public class EventController : Controller
    {
        //
        // GET: /Event/

        public ActionResult Index()
        {
            return View();
        }

        public ActionResult Add()
        {
            try
            {
                Models.ITeamManage teamManage = new Models.TeamManage();
                List<Models.TeamModels> team_list;
                team_list = teamManage.select("");
                List<string> team = new List<string>();
                foreach (Models.TeamModels t in team_list)
                {
                    team.Add(t.City+t.Name);
                }
                ViewData["TeamListHost"] = new SelectList(team);
                ViewData["TeamListVisit"] = new SelectList(team);
                List<int> year = new List<int>();
                List<int> mouth = new List<int>();
                List<int> day = new List<int>();
                for (int i = 2012; i < 2015; i++)
                {
                    year.Add(i);
                }
                for (int i = 1; i <= 12; i++)
                {
                    mouth.Add(i);
                }
                for (int i = 1; i <= 31; i++)
                {
                    day.Add(i);
                }
                ViewData["Year"] = new SelectList(year);
                ViewData["Mouth"] = new SelectList(mouth);
                ViewData["Day"] = new SelectList(day);
                return View();
            }
            catch (Exception ex)
            {
                Response.Write(ex.Message);
                return View();
            }
            
        }

        public void AddAction()
        {
            Models.EventManage eventmanage = new Models.EventManage();
            Models.TeamManage teammanage = new Models.TeamManage();
        }

    }
}
