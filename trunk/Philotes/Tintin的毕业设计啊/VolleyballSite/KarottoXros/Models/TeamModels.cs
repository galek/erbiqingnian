using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Data.Sql;
using System.Data.SqlClient;
using System.Data.SqlTypes;
namespace KarottoXros.Models
{
    public class TeamModels
    {
        public int Id { set; get; }
        public string Name { set; get; }
        public string City { set; get; }
        public int location { set; get; }
        public string Info { set; get; }
        public List<Models.PlayerModels> player_list;
        public TeamModels()
        {
            player_list = new List<PlayerModels>();
        }
    }

    public interface ITeamManage
    {
        string insert(TeamModels team);
        string delete(int id);
        string alter(int id, TeamModels team);
        List<string> selectlocation();
        List<TeamModels> select(string atter);
        Models.TeamModels selectById(int id);
        int selectIdByName(string name);
        
    }

    public class TeamManage : ITeamManage
    {
        public KarottoXros.Code.SqlManager sql;
        public TeamManage() { sql = new Code.SqlManager(); }
        public string insert(TeamModels team)
        {
            try {
                string insert_str = "insert into TeamTable(Name,City,Location,Info) values('" + team.Name + "','" + team.City + "'," + team.location + ",'" + team.Info + "')";
                int n = sql.ExecuteNonQuery(insert_str);
                if (n > 0)
                    return "<script>alert('ok');</script>";
                else
                    return "<script>alert('才不ok呢');</script>";
            }
            catch (SqlException ex)
            {
                return "<script>alert('"+ex.Message+"');</script>";
            }
           
        }

        public string delete(int id)
        {
            return null;
            
        }

        public string alter(int id, TeamModels team)
        {
            try
            {
                string alt_str = "update [TeamTable] set [Name] = '"+team.Name+"',[City]='"+team.City+"',[Info]='"+team.Info+"',[Location]="+team.location+" where [Id] = "+id+"";
                int n = sql.ExecuteNonQuery(alt_str);
                if (n > 0)
                    return "<script>alert('我去了成功了');</script>";
                else
                    return "<script>alert('我去了怎么失败了呢？');</script>";
            }
            catch (SqlException ex)
            {
                return ex.Message;
            }
            
        }

        public List<TeamModels> select(string atter)
        {

            try
            {
                SqlDataReader reader;
                string select = "SELECT [Id], [Name],[City] FROM [TeamTable]";
                reader = sql.ExecuteReader(select);
                List<Models.TeamModels> team_list = new List<Models.TeamModels>();
                while (reader.Read())
                {
                    TeamModels team = new TeamModels();
                    team.Id = reader.GetInt32(0);
                    team.Name = reader.GetString(1);
                    team.City = reader.GetString(2);
                    team_list.Add(team);
                }

                return team_list;
            }
            catch (SqlException ex)
            {
                throw ex;
            }
        }

        public List<string> selectlocation()
        {
            try
            {
                string str = "SELECT [Name] FROM [LocationTable]";
                List<string> l_str_name = new List<string>();
                SqlDataReader reader = sql.ExecuteReader(str);
                while (reader.Read())
                {
                    string s = reader.GetString(0);
                    l_str_name.Add(s);
                }
                return l_str_name;
            }
            catch (SqlException ex)
            {
                throw ex;
            }
            
        }

        public Models.TeamModels selectById(int id)
        {
            try
            {
                Models.TeamModels team = new TeamModels();
                string str = "SELECT [Name], [City], [Info], [Location] FROM [TeamTable] where [Id] = "+id+"";
                SqlDataReader reader = sql.ExecuteReader(str);
                reader.Read();
                team.Id = id;
                team.Name = reader.GetString(0);
                team.City = reader.GetString(1);
                team.Info = reader.GetString(2);
                team.location = reader.GetInt32(3);
                return team;
            }
            catch (SqlException ex)
            {
                throw ex;
            }
        }

        public int selectIdByName(string name)
        {
            return 0;
        }
    }
}