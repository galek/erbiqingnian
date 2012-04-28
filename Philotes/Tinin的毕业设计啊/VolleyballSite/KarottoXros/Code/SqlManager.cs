using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Data.Sql;
using System.Data.SqlClient;
using System.Data.SqlTypes;

namespace KarottoXros.Code
{
    public class SqlManager
    {
        SqlConnection scon;
        SqlCommand scmd;
        public SqlManager()
        {
            try
            {
                scon = new SqlConnection(@"Data Source=.\SQLEXPRESS;AttachDbFilename=|DataDirectory|\KDBXros.mdf;Integrated Security=True;User Instance=True");
                scon.Open();
                scmd = scon.CreateCommand();
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        public string ExecuteLogin(string name, string pwd)
        {
            scmd.CommandText = "select count(*) from UserTable where UserName =@user_name and UserPassword=@user_passwd";
            scmd.Parameters.Add("@user_name", System.Data.SqlDbType.VarChar, 20).Value = name;
            scmd.Parameters.Add("@user_passwd", System.Data.SqlDbType.VarChar, 30).Value = pwd;
            int n = Convert.ToInt32(scmd.ExecuteScalar());
            if (n != 0)
            {
                scmd.CommandText = "select UserRole from UserTable where UserName ='"+name+"'";
                string role = Convert.ToString(scmd.ExecuteScalar());
                return role;
            }
            else
            {
                return null;
            }
            
        }

        public SqlDataReader ExecuteReader(string strcmd)
        {
            scmd.CommandText = strcmd;
            return scmd.ExecuteReader();
        }

        public object ExecuteScalar(string strcmd)
        {
            scmd.CommandText = strcmd;
            return scmd.ExecuteScalar();
        }

        public int ExecuteNonQuery(string strcmd)
        {
            scmd.CommandText = strcmd;
            return scmd.ExecuteNonQuery();
        }

        ~SqlManager()
        {
        }
    }
}