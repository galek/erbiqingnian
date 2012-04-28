using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.IO;
using System.Text;
namespace KarottoXros.Code
{
    public class Xros
    {

        static public string FilePath = @"F:\KBasketBallXros\KarottoXros\KarottoXros\News\";

        static public string GetFile(string filename)
        {
            try
            {
                string contents = File.ReadAllText(filename,Encoding.GetEncoding("GB2312"));
                //FileInfo file = new FileInfo(filename);
                //file.OpenRead();
                //file.
                return contents;
            }
            catch (Exception ex)
            {
                return ex.Message;
            }
            
        }

        static public string DeleteFile(string filename)
        {
            try
            {
                File.Delete(filename);
                return "删除也成功";
            }
            catch (Exception ex)
            {
                return ex.Message;
            }
            
        }

        static public string UpdateFile(string filename, string content)
        {
            //StreamWriter streamwrite = null;
            //StreamReader streamread = null;
            try
            {
                File.WriteAllText(filename, content, Encoding.GetEncoding("GB2312"));
                return "成功";
            }
            catch (Exception ex)
            {
                return ex.Message;
            }
        }

        static public string WriteFile(string filename,string content)
        {
            try
            {
                File.WriteAllText(filename,content, Encoding.GetEncoding("GB2312"));
                return "成功了";
            }
            catch (Exception ex)
            {
                return ex.Message;
            }
            
        }
    }
}