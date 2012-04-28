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
-->
</style>
</head>
<body>

<script language="javascript" type="text/javascript">
    $(document).read(function () {

    });
   
</script>
    <div>
    
      <div align="center">
	  <div ><a href="/BG/Index?Length=4"><img src="../../image/ba1.png" width="960" height="120" border="0" /></a></div>
	  <div><table width="960" border="0" cellspacing="0" cellpadding="0">
  <tr>
    <td width="290" rowspan="2" valign="middle"><div align="center"><img src="../../image/batmp1.png" width="283" height="79" /></div></td>
    <td><table width="670" border="0" cellspacing="0" cellpadding="5">
      <tr>
	    <td valign="top">
		  <div align="left" class="TimeCss"><%=ViewData["Date"] %>		    </div></td>
        <td><div align="right">
          <%using (Html.BeginForm("Index", "Home", FormMethod.Post)){%>
          <%=Html.TextBox("strkey") %>
          <%=Html.DropDownList("ItemKey",new SelectList(new string[]{"新闻","数据","球员"})) %>
          <input type =submit value = "提交">
          <%}%>
        </div></td>
      </tr>
    </table></td>
  </tr>
  <tr>
    <td>
      <div id ="alink" align="left">
	  <table>
	  <tr>
	  <td class="Xros">
	    <div align="center"><%=Html.ActionLink("首页", "About", "Home",new{@class = "ZiA"})%>	        </div></td>
	  <td class="Xros">
	    <div align="center"><%=Html.ActionLink("新闻", "About", "Home", new { @class = "ZiA"})%>	        </div></td>
	  <td class="Xros">
	    <div align="center"><%=Html.ActionLink("数据", "About", "Home", new { @class = "ZiA" })%>	        </div></td>
	  <td class="Xros">
	    <div align="center"><%=Html.ActionLink("赛程", "About", "Home", new { @class = "ZiA" })%>	        </div></td>
	  <td class="Xros">
	    <div align="center"><%=Html.ActionLink("球队", "About", "Home", new { @class = "ZiA" })%>	        </div></td>
	  <td class="Xros">
	    <div align="center"><%=Html.ActionLink("球员", "About", "Home", new { @class = "ZiA" })%>	        </div></td>
	  </tr>
	  </table>
      		
        </div></td>
  </tr>
</table>
</div>
	  </div>
    </div>
    
    <script language="javascript" type="text/javascript">
     
        $(".ZiA").mouseover(function () { $(this).css({ "font-size": "20px", "color": "#666666" }); });
   
    </script>
    <script language="javascript" type="text/javascript">

        $(".Xros").mouseover(function () { $(this).css({ "background-image: ": "url(../image/bga2.png)" }); });
   
    </script>
    <script language="javascript" type="text/javascript">
        $(".Xros").mouseout(function () { $(this).css({ "background-image: ": "url(../image/bga.png)" }); });
   
    </script>
    <script language="javascript" type="text/javascript">
       
        $(".ZiA").mouseout(function () { $(this).css({ "font-size": "18px", "color": "#FFFFFF" }); });
    </script>
    

    
</body>
</html>
