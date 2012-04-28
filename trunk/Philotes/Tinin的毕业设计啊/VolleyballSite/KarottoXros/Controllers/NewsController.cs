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
using System.IO;

namespace KarottoXros.Controllers
{
    public class NewsController : Controller
    {
        public ActionResult Index()
        {
            
            try
            {
                Models.INewsManage newsmanage = new Models.NewsManage();
                List<Models.NewsModels> news_list;
                news_list = newsmanage.select("");
                ViewData["NewsList"] = news_list;
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
            string nid = Convert.ToString(Request.QueryString["NewsId"]);
            ViewData["contents"] = Xros.GetFile(Code.Xros.FilePath+nid+".txt");
            ViewData["NewsId"] = nid;

            return View();
        }

        public ActionResult UpLoad()
        {
            return View();
        }

        public void UpLoadAction()
        {
            try 
            {
                Models.NewsManage newsmanage = new Models.NewsManage();
                Models.NewsModels news = new Models.NewsModels();
                news.Topic = Convert.ToString(Request.Form["topic"]);
                news.Path = "tmp";
                newsmanage.insert(news);
                string id = newsmanage.GetNews(news.Topic);
                string content = Convert.ToString(Request.Form["content"]);
                Code.Xros.WriteFile(Code.Xros.FilePath+id+".txt", content);
            }
            catch (Exception ex)
            {
                Response.Write(String.Format("<script>alert('{0}');</script>",ex.Message));
            }
            
        }

        public ActionResult Alter()
        {
            Models.NewsManage newsmanage = new Models.NewsManage();
            string nid = Convert.ToString(Request.QueryString["NewsId"]);
            int id = Convert.ToInt32(nid);
            ViewData["contents"] = Xros.GetFile(Code.Xros.FilePath + nid + ".txt");
            ViewData["NewsId"] = nid;
            ViewData["NewsTopic"] = newsmanage.GetNews(id);
            
            return View();
        }

        public void AlterAction()
        {
            try
            {
                int nid = Convert.ToInt32(Request.Form["NewsId"]);
                string contents = Request.Form["contents"];
                string alert = Xros.UpdateFile(Code.Xros.FilePath + nid + ".txt", contents);
                Response.Write(String.Format("<script>alert('{0}');</script>",alert));
                Redirect("Index");
            }
            catch (Exception ex)
            {
                Response.Write(String.Format("<script>alert('{0}');</script>",ex.Message));
                Redirect("Index");
            }
        }
    }
}
