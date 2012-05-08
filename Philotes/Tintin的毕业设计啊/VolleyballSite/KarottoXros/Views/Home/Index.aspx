<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
<script src="../../Scripts/jquery-1.4.1.min.js" type="text/javascript"></script>
    <title></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" /><style type="text/css">
<!--
body {
	background-image: url(../../image/bg1.png);
	background-repeat: repeat-x;
}
-->
</style>
<link href="../../CSS/Xros.css" rel="stylesheet" type="text/css" />
<style type="text/css">
<!--
.STYLE1 {
	font-size: 14;
	color: #CCCCCC;
}
    .style1
    {
        width: 278px;
    }
</style>
-->
</style>
</head>
<body>
<div align="center">
	    <div ><a href="/BG/Index?Length=4"><img src="../../image/ba1.png" width="937" height="147" border="0" /></a></div>
        <div>
        <table border="0" cellspacing="0" cellpadding="0" style="width: 937px">
            <tr>
                <td class="Xros2"><div align="center" id="curdate"></div></td>
                <td class="Xros"><div align="center"><%=Html.ActionLink("首页", "About", "Home",new{@class = "ZiA"})%></div></td>
                <td class="Xros"><div align="center"><%=Html.ActionLink("新闻", "About", "Home", new { @class = "ZiA"})%></div></td>
                <td class="Xros"><div align="center"><%=Html.ActionLink("数据", "About", "Home", new { @class = "ZiA" })%></div></td>
                <td class="Xros"><div align="center"><%=Html.ActionLink("赛程", "About", "Home", new { @class = "ZiA" })%></div></td>
                <td class="Xros"><div align="center"><%=Html.ActionLink("球队", "About", "Home", new { @class = "ZiA" })%></div></td>
                <td class="Xros"><div align="center"><%=Html.ActionLink("球员", "About", "Home", new { @class = "ZiA" })%></div></td>
                <td class="Xros"><div align="center"><%=Html.ActionLink("登录", "About", "Home", new { @class = "ZiA" })%></div></td>
            </tr>
    </table>
    </div>
</div>

    <script language="javascript" type="text/javascript">
     
        $(".ZiA").mouseover(function () { $(this).css({ "font-size": "20px", "color": "#666666" }); });
   
    </script>
    <script language="javascript" type="text/javascript">
       
        $(".ZiA").mouseout(function () { $(this).css({ "font-size": "18px", "color": "#000000" }); });
    </script>
    <script>                    
        var now = new Date(2012, 05-1, 08, 19, 16, 08);
        var week=new Array(7); 
　　        week[0]="天"; 
　　        week[1]="一"; 
　　        week[2]="二"; 
　　        week[3]="三"; 
　　        week[4]="四"; 
　　        week[5]="五"; 
　　        week[6]="六"; 
        var datestr=(now.getMonth()+1)+"月"+now.getDate()+"日 星期"+week[now.getDay()];
        document.getElementById("curdate").innerHTML = datestr;
    </script>
</body>
</html>
