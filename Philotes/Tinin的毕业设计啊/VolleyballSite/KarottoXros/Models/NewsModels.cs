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
namespace KarottoXros.Models
{
    public class NewsModels
    {
        public NewsModels()
        {
            Topic = "无标题";
            Path = "tmp";
            Author = "no";

        }
        public int Id { set; get; }
        public string Topic { set; get; }
        //public string Content { set; get; }
        public string Path { set; get; }
        public string Date { set; get; }
        public string Author { set; get; }
    }
     

    public interface INewsManage
    {
        string insert(NewsModels news);
        string delete(NewsModels news);
        string alter(int Id, NewsModels news);
        List<NewsModels> select(string attribute);
        string GetNews(string news_topic);
        string GetNews(int news_id);
    }

    public class NewsManage : INewsManage
    {
        Code.SqlManager sql;
        public NewsManage()
        {
            sql = new Code.SqlManager();
        }

        public string insert(NewsModels news)
        {

            try
            {
                string date = DateTime.Today.Year + "-" + DateTime.Today.Month + "-" + DateTime.Today.Day;
                string insertstr = "insert into NewsTable(topic,date,author,path) values('"+news.Topic+"','"+date+"','"+news.Author+"','"+news.Path+"')";
                int tempn = sql.ExecuteNonQuery(insertstr);
                if (tempn > 0)
                    return("<script>alert('新闻发表成功');</script>");
                else
                    return("<script>alert('意外情况失败了');</script>");
            }
            catch (SqlException ex)
            {
                return ("<script>alert('"+ex.Message+"');</script>");
            }
        }


        public string delete(NewsModels news)
        {
            try
            {
                string insertstr = "delete NewsTable where id = '" + news.Id + "'";
                int tempn = sql.ExecuteNonQuery(insertstr);
                if (tempn > 0)
                    //Response.Write("<script>alert('恭喜你注册成功');window.location.href ='houseadd.aspx'</script>");
                    return ("<script>alert('删除成功');</script>");
                else
                    return ("<script>alert('操作失败');</script>");
            }
            catch (SqlException ex)
            {
                return ("<script>alert('" + ex.Message + "');</script>");
            }
        }

        public string alter(int Id, NewsModels news)
        {
            try
            {
                string updeteuser = "update NewsTable set topic = '" + news.Topic + "',author = '"+news.Author+"' where id = '" + news.Id + "'";
                int tempn = sql.ExecuteNonQuery(updeteuser);
                if (tempn == 0)
                {
                    return "<script>alert('更新不成');</script>";
                }
                else
                {
                    return "<script>alert('ok你成功了');</script>";
                }
            }
            catch (SqlException ex)
            {
                return ("<script>alert('" + ex.Message + "');</script>");
            }
        }

        public List<NewsModels> select(string attribute)
        {
            try
            {
                List<NewsModels> list_news = new List<NewsModels>();
                System.Data.SqlClient.SqlDataReader reader;
                string select_str_all = "SELECT [id], [topic], [author], [path] FROM [NewsTable]";
                reader = sql.ExecuteReader(select_str_all);
                
                while (reader.Read())
                {
                    NewsModels news = new NewsModels();
                    news.Id = reader.GetInt32(0);
                    news.Topic = reader.GetString(1);
                    news.Author = reader.GetString(2);
                    news.Path = reader.GetString(3);
                    list_news.Add(news);
                }

                return list_news;
            }
            catch (Exception ex)
            {
                throw ex;
            }
            
        }
        
        

        public string GetNews(string news_topic)
        {
            try
            {
                string select_str_id = "SELECT [id] FROM [NewsTable] where [topic] = '"+news_topic+"' and [path] = 'tmp'";
                string id = Convert.ToString(sql.ExecuteScalar(select_str_id));
                string updetenews = "update NewsTable set path = '"+Code.Xros.FilePath+id+".txt"+"' where id = " + id + "";
                int tempn = sql.ExecuteNonQuery(updetenews);
                if (tempn == 0)
                {
                    return "<script>alert('意外情况你没成功');</script>";
                }
                else
                    return id;
            }
            catch (Exception ex)
            {
                return ex.Message;
            }
        }

        public string GetNews(int news_id)
        {
            try
            {
                string select_str_id = "SELECT [topic] FROM [NewsTable] where [id] = " + news_id;
                string str_topic = Convert.ToString(sql.ExecuteScalar(select_str_id));
                return str_topic;
            }
            catch (Exception ex)
            {
                return ex.Message;
            }
        }

        

    }
}