using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using KarottoXros.Code;
using System.Data.Sql;
using System.Data.SqlClient;
using System.Data.SqlTypes;
namespace KarottoXros.Models
{
    public enum location {PG,SG,SF,PF,C}
    public class PlayerModels
    {
        public PlayerModels() 
        {
            Name = "None";
            ChineseName = "还没有名字";
            Hight = 198;
            Weight = 100;
            Birthday = "1990-1-1";
            University = "Don't Know";
            Info = " ";
            MainLocation = "SG";
            Location = new int[5]{ 0, 0, 0, 0, 0 };
            
        }
        public int ID { set; get; }
        public string Name { set; get; }
        public string ChineseName { set; get; }
        public int Hight { set;get;}
        public int Weight { set; get; }
        public string Birthday { set; get; }
        public string University { set; get; }
        public string Info { set; get; }
        public int[] Location;
        public string MainLocation { set; get; }
        public int Team { set; get; }
        public static string Toky(location l)
        {
            switch(l)
            {
                case location.C:  return "C";
                case location.PF: return "PF";
                case location.SF: return "SF";
                case location.SG: return "SG";
                case location.PG: return "PG";
                default: return null;
            }
        }

        public static int TokyI(location l)
        {
            switch (l)
            {
                case location.C:  return 5;
                case location.PF: return 4;
                case location.SF: return 3;
                case location.SG: return 2;
                case location.PG: return 1;
                default: return 1;
            }
        }

        public void setLocation(int PG,int SG,int SF,int PF,int C)
        {
            Location[0] = PG;
            Location[1] = SG;
            Location[2] = SF;
            Location[3] = PF;
            Location[4] = C;
        }
    }

    public interface IPlayerManage
    {
        string insert(PlayerModels player);
        string delete(int id);
        string alter(int id, PlayerModels player);
        List<PlayerModels> select(string atter);
        int getplayerID(PlayerModels player);
        PlayerModels selectById(int i);
    }

    public class PlayerManage : IPlayerManage
    {
        Code.SqlManager sql;
        public PlayerManage()
        {
            sql = new Code.SqlManager();
        }
        public string insert(PlayerModels player)
        {
            try
            {
                string date = DateTime.Today.Year + "-" + DateTime.Today.Month + "-" + DateTime.Today.Day;
                string insertstr = "insert into PlayerTable(Name,ChineseName,Hight,Weight,Birthday,University,Info,MainLocation,Team) values('" + player.Name + "','" + player.ChineseName + "'," + player.Hight + "," + player.Weight + ",'" + player.Birthday + "','" + player.University + "','" + player.Info + "','" + player.MainLocation+ "',"+player.Team+")";
                int tempn = sql.ExecuteNonQuery(insertstr);
                if (tempn > 0)
                {
                    int id = getplayerID(player);
                    string insert_location = "insert into PlayerLocation(Id,PG,SG,SF,PF,C) values(" + id + "," + player.Location[0] + "," + player.Location[1] + "," + player.Location[2] + "," + player.Location[3] + "," + player.Location[4] + ")";
                    int tt = sql.ExecuteNonQuery(insert_location);
                    if(tt>0)
                        return("<script>alert('新球员添加成功');</script>");
                    else
                        return ("<script>alert('位置分配失败了');</script>");
                }
                else
                    return ("<script>alert('意外情况失败了');</script>");
            }
            catch (SqlException ex)
            {
                return ("<script>alert('" + ex.Message + "');</script>");
            }
        }
        public string delete(int id)
        {
            try
            {
                string deletestr1 = "delete PlayerTable where [Id] = " + id + "";
                string deleteste = "delete PlayerLocation where [Id] = " + id + "";
                int tempn = sql.ExecuteNonQuery(deletestr1);
                int tn = sql.ExecuteNonQuery(deleteste);
                if (tempn > 0)
                {
                    if(tn>0)
                        return ("<script>alert('删除成功');</script>");
                    else
                        return ("<script>alert('意外意外');</script>");
                }
                else
                    return ("<script>alert('失败');</script>");
            }
            catch (SqlException ex)
            {
                return ("<script>alert('" + ex.Message + "');</script>");
            }
        }
        public string alter(int id, PlayerModels player)
        {
            try
            {
                string updete1 = "update PlayerTable set Name = '"+player.Name+"',ChineseName = '"+player.ChineseName+"',Hight = "+player.Hight+",Weight="+player.Weight+",University='"+player.University+"',Birthday = '"+player.Birthday+"',Info = '"+player.Info+"',MainLocation = '"+player.MainLocation+"' where Id = "+id+"";
                string update2 = "update PlayerLocation set PG=" + player.Location[0] + ",SG=" + player.Location[1] + ",SF=" + player.Location[2] + ",PF=" + player.Location[3] + ",C=" + player.Location[4] + " where Id="+id+"";
                int temp1 = sql.ExecuteNonQuery(updete1);
                if (temp1 == 0)
                {
                    return "<script>alert('更新不成');</script>";
                }
                else
                {
                    int temp2 = sql.ExecuteNonQuery(update2);
                    if (temp2 == 0)
                    {
                        return "<script>alert('这是什么意外？');</script>";
                    }
                    else
                        return "<script>alert('ok你成功了');</script>";
                }
            }
            catch (SqlException ex)
            {
                return ("<script>alert('" + ex.Message + "');</script>");
            }
        }
        public List<PlayerModels> select(string atter)
        {
            SqlDataReader reader;
            string select = "SELECT [Id], [Name],[ChineseName] FROM [PlayerTable]";
            reader = sql.ExecuteReader(select);
            List<Models.PlayerModels> player_list = new List<PlayerModels>();
            while (reader.Read())
            {
                PlayerModels player = new PlayerModels();
                player.ID = reader.GetInt32(0);
                player.Name = reader.GetString(1);
                player.ChineseName = reader.GetString(2);
                player_list.Add(player);
            }

            return player_list;
        }
        public int getplayerID(PlayerModels player)
        {
            try
            {
                string select_str_id = "SELECT [Id] FROM [PlayerTable] where [Name] = '" +player.Name+ "'";
                int id = Convert.ToInt32(sql.ExecuteScalar(select_str_id));
                return id;
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        public PlayerModels selectById(int id)
        {
            try 
            {
                string select = "SELECT PlayerTable.Id,PlayerTable.Name,PlayerTable.ChineseName,PlayerTable.BirthDay,PlayerTable.MainLocation,PlayerTable.Info,PlayerTable.University,PlayerLocation.PG,PlayerLocation.SG,PlayerLocation.SF,PlayerLocation.PF,PlayerLocation.C,PlayerTable.Hight,PlayerTable.Weight FROM [PlayerTable],[PlayerLocation] where PlayerTable.Id = " + id + " and PlayerTable.Id = PlayerLocation.Id";
                SqlDataReader reader;
                reader = sql.ExecuteReader(select);
                reader.Read();
                PlayerModels player = new PlayerModels();
                player.ID = reader.GetInt32(0);
                player.Name = reader.GetString(1);
                player.ChineseName = reader.GetString(2);
                player.Birthday = reader.GetString(3);
                player.MainLocation = reader.GetString(4);
                player.Info = reader.GetString(5);
                player.University = reader.GetString(6);
                player.Location[0] = reader.GetInt32(7);
                player.Location[1] = reader.GetInt32(8);
                player.Location[2] = reader.GetInt32(9);
                player.Location[3] = reader.GetInt32(10);
                player.Location[4] = reader.GetInt32(11);
                player.Hight = reader.GetInt32(12);
                player.Weight = reader.GetInt32(13);
                return player;
            }catch(SqlException ex)
            {
                throw ex;
            }
            
        }
    }
}