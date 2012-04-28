using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Data.Sql;
using System.Data.SqlClient;
using System.Data.SqlTypes;

namespace KarottoXros.Models
{
    public class UserModels
    {

        public string UserName { set; get; }
        public string UserPassword { set; get; }
        public string UserRole { set; get; }

    }

    public interface IUserManage
    {
        string insert(UserModels user);
        string delete(UserModels user);
        string alter(string username, UserModels user);
        UserModels getUser(string user_name);
        List<UserModels> select(string attribute);
        string Login(string user_name,string user_password);
    }

    public class UserManage : IUserManage
    {
        Code.SqlManager sql;
        public UserManage()
        {
            sql = new Code.SqlManager();
        }
        
        public string insert(UserModels user)
        {

            try
            {
                string selectstr = "select count(*) from UserTable where userName = '" + user.UserName + "'";
                int tempn = Convert.ToInt32(sql.ExecuteScalar(selectstr));
                if (tempn != 0)
                {
                    return "<script>alert('该用户已存在,你那个智商应该知道怎么解决吧');</script>";
                }
                string insertstr = "insert into UserTable(userName,userRole,userPassword) values('" + user.UserName + "','" + user.UserRole + "','" + user.UserPassword + "')";
                tempn = sql.ExecuteNonQuery(insertstr);
                if (tempn > 0)
                    //Response.Write("<script>alert('恭喜你注册成功');window.location.href ='houseadd.aspx'</script>");
                    return("<script>alert('注册成功');</script>");
                else
                    return("<script>alert('失败了');</script>");
            }
            catch (SqlException ex)
            {
                return ("<script>alert('"+ex.Message+"');</script>");
            }
        }


        public string delete(UserModels user)
        {
            try
            {
                string selectstr = "select count(*) from UserTable where userName = '" + user.UserName + "'";
                int tempn = Convert.ToInt32(sql.ExecuteScalar(selectstr));
                if (tempn == 0)
                {
                    return "<script>alert('该用不存在,你那个智商应该知道怎么解决吧');</script>";
                }
                string insertstr = "delete UserTable where UserName = '"+user.UserName+"'";
                tempn = sql.ExecuteNonQuery(insertstr);
                if (tempn > 0)
                    //Response.Write("<script>alert('恭喜你注册成功');window.location.href ='houseadd.aspx'</script>");
                    return ("<script>alert('删除成功');</script>");
                else
                    return ("<script>alert('失败');</script>");
            }
            catch (SqlException ex)
            {
                return ("<script>alert('" + ex.Message + "');</script>");
            }
        }

        public string alter(string username, UserModels user)
        {
            try
            {
                string updeteuser = "update UserTable set UserName = '" + user.UserName + "' ,UserPassword = '" + user.UserPassword + "' where UserName = '" +username+ "'";
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

        public UserModels getUser(string user_name)
        {
            return null;
        }

        public List<UserModels> select(string attribute)
        {
            try
            {
                List<UserModels> list_user = new List<UserModels>();
                System.Data.SqlClient.SqlDataReader reader;
                string select_str_all = "SELECT [UserName], [UserRole] FROM [UserTable]";
                reader = sql.ExecuteReader(select_str_all);
                
                while (reader.Read())
                {
                    UserModels user = new UserModels();
                    user.UserName = reader.GetString(0);
                    user.UserRole = reader.GetString(1);
                    list_user.Add(user);
                }
                
                return list_user;
            }
            catch (Exception ex)
            {
                throw ex;
            }
            
        }

        public string Login(string user_name,string user_password)
        {
            return sql.ExecuteLogin(user_name, user_password);
        }
    }
}