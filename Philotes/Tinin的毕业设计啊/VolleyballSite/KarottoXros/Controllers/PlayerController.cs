using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace KarottoXros.Controllers
{
    public class PlayerController : Controller
    {
        //
        // GET: /Player/

        public ActionResult Index()
        {
            return View();
        }

        public ActionResult AddPlayer()
        {
            List<int> year = new List<int>();
            List<int> mouth = new List<int>();
            List<int> day = new List<int>();
            string[] loc = new string[5] { "PG", "SG", "SF", "PF", "C" };
            for (int i = 1960; i < 2012; i++)
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
            ViewData["Loc"] = new SelectList(loc); ;
            return View();
        }

        public void AddPlayerAction()
        {
            Models.PlayerModels player = new Models.PlayerModels();
            Models.PlayerManage playermanage = new Models.PlayerManage();
            player.Name = Request.Form["Name"];
            player.ChineseName = Request.Form["ChineseName"];
            player.Hight = Convert.ToInt32(Request.Form["Hight"]);
            player.Weight = Convert.ToInt32(Request.Form["Weight"]);
            string birth = Request.Form["Year"]+"-"+Request.Form["Mouth"]+"-"+Request.Form["Day"];
            player.Birthday = birth;
            player.University = Request.Form["University"];
            player.MainLocation = Request.Form["Loc"];
            player.Location[0] = getLoc(Request.Form["PG"]);
            player.Location[1] = getLoc(Request.Form["SG"]);
            player.Location[2] = getLoc(Request.Form["SF"]);
            player.Location[3] = getLoc(Request.Form["PF"]);
            player.Location[4] = getLoc(Request.Form["C"]);
            string o = playermanage.insert(player);
            Response.Write(o);
        }

        private int getLoc(string isok)
        {
            if (!isok.Equals("false"))
                return 1;
            else return 0;
        }

        public ActionResult List()
        {
            try
            {
                Models.IPlayerManage playermanage = new Models.PlayerManage();
                List<Models.PlayerModels> player_list;
                player_list = playermanage.select("");
                ViewData["PlayerList"] = player_list;
                return View();
            }
            catch (Exception ex)
            {
                Response.Write(ex.Message);
                return View();
            }
        }

        public void delete()
        {
            try
            {
                Models.PlayerManage playermanage = new Models.PlayerManage();
                int pid = Convert.ToInt32(Request.QueryString["PlayerID"]);
                string o = playermanage.delete(pid);
                Response.Write(o);
            }
            catch (Exception ex)
            {
                Response.Write(ex.Message);
            }
        }

        public ActionResult Show()
        {
            try
            {
                Models.PlayerManage playermanage = new Models.PlayerManage();
                int pid = Convert.ToInt32(Request.QueryString["PlayerID"]);
                Models.PlayerModels player = playermanage.selectById(pid);
                ViewData["Pid"] = pid;
                ViewData["PlayerName"] = player.Name;
                ViewData["ChineseName"] = player.ChineseName;
                ViewData["Hight"] = player.Hight;
                ViewData["Weight"] = player.Weight;
                ViewData["University"] = player.University;
                ViewData["MainLocation"] = player.MainLocation;
                List<string> loc_str = getLocationString(player);
                ViewData["LocationList"] = loc_str;
                
                return View();
            }
            catch (Exception ex)
            {
                Response.Write(ex.Message);
                return View();
            }
        }

        private List<string> getLocationString(Models.PlayerModels player)
        {
            List<string> str = new List<string>();
            if (player.Location[0] != 0)
                str.Add("PG");
            if (player.Location[1] != 0)
                str.Add("SG");
            if (player.Location[2] != 0)
                str.Add("SF");
            if (player.Location[3] != 0)
                str.Add("PF");
            if (player.Location[4] != 0)
                str.Add("C");
            return str;
        }

        public ActionResult Alter()
        {
            List<int> year = new List<int>();
            List<int> mouth = new List<int>();
            List<int> day = new List<int>();
            string[] loc = new string[5] { "PG", "SG", "SF", "PF", "C" };
            for (int i = 1960; i < 2012; i++)
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
            ViewData["Loc"] = new SelectList(loc);
            try
            {
                Models.PlayerManage playermanage = new Models.PlayerManage();
                int pid = Convert.ToInt32(Request.QueryString["PlayerID"]);
                Models.PlayerModels player = playermanage.selectById(pid);
                ViewData["Pid"] = pid;
                ViewData["PlayerName"] = player.Name;
                ViewData["ChineseName"] = player.ChineseName;
                ViewData["Hight"] = player.Hight;
                ViewData["Weight"] = player.Weight;
                ViewData["University"] = player.University;
                ViewData["MainLocation"] = player.MainLocation;
                List<string> loc_str = getLocationString(player);
                ViewData["LocationList"] = loc_str;

                return View();
            }
            catch (Exception ex)
            {
                Response.Write(ex.Message);
                return View();
            }
        }

        public void AlterAction()
        {
            
            try
            {
                Models.PlayerModels player = new Models.PlayerModels();
                Models.PlayerManage playermanage = new Models.PlayerManage();
                player.ID = Convert.ToInt32(Request.Form["Pid"]);
                player.Name = Request.Form["PlayerNameName"];
                player.ChineseName = Request.Form["ChineseName"];
                player.Hight = Convert.ToInt32(Request.Form["Hight"]);
                player.Weight = Convert.ToInt32(Request.Form["Weight"]);
                string birth = Request.Form["Year"] + "-" + Request.Form["Mouth"] + "-" + Request.Form["Day"];
                player.Birthday = birth;
                player.University = Request.Form["University"];
                player.MainLocation = Request.Form["Loc"];
                player.Location[0] = getLoc(Request.Form["PG"]);
                player.Location[1] = getLoc(Request.Form["SG"]);
                player.Location[2] = getLoc(Request.Form["SF"]);
                player.Location[3] = getLoc(Request.Form["PF"]);
                player.Location[4] = getLoc(Request.Form["C"]);
                string o = playermanage.alter(player.ID, player);
                
                Response.Write(o);
            }
            catch (Exception ex)
            {
                Response.Write(String.Format("<script>alert('{0}');</script>",ex.Message));
            }
            
        }


    }
}
