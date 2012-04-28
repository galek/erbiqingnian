using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Data.Sql;
using System.Data.SqlClient;
using System.Data.SqlTypes;
namespace KarottoXros.Models
{
    public class EventModels
    {
        public int Id { set; get; }
        public string date { set; get; }
        public int hostId { set; get; }
        public int visitId { set; get; }
        public int tag { set; get; }
        public List<int> hostPlayer;
        public List<int> visitPlayer;
        public EventModels() 
        {
            tag = 0;
            visitPlayer = new List<int>();
            hostPlayer = new List<int>();
        }
    }

    public class PlayerEventModels
    {
        public int EId { set; get; }
        public int PlayerId { set; get; }
        public int Point { set; get; }
        public int Backboard { set; get; }
        public int Assist { set; get; }
        public int time { set; get; }
        
    }

    public class EventManage
    {
        Code.SqlManager sql;
        public EventManage()
        {
            sql = new Code.SqlManager();
        }
        public string insert(EventModels Event)
        {
            try
            {
                //string instr = "insert into EventTable(EId,PlayerId,Point,Backboard,Assist,time) values("+Event.Eid+","","","","","")";
                string instr = "insert into EventTable(date,hostId,visitId) values('" + Event.date + "'," + Event.hostId + "," + Event.visitId + ")";
                int n = sql.ExecuteNonQuery(instr);
                if (n > 0)
                    return "<script>alert('添加成功');</script>";
                else
                    return "<script>alert('有意外发生');</script>";
            }
            catch (SqlException ex)
            {
                return "<script>alert('"+ex.Message+"');</script>";
            }
        }

        public string insert(PlayerEventModels Event)
        {
            try
            {
                string instr = "insert into PlayerEventTable(EId,PlayerId,Point,Backboard,Assist,time) values(" + Event.EId + "," + Event.PlayerId + "," + Event.Point + "," + Event.Backboard + "," + Event.Assist + "," + Event.time + ")";
                int n = sql.ExecuteNonQuery(instr);
                if (n > 0)
                    return "<script>alert('添加成功');</script>";
                else
                    return "<script>alert('有意外发生');</script>";
            }
            catch (SqlException ex)
            {
                return "<script>alert('" + ex.Message + "');</script>";
            }
        }
    }
}