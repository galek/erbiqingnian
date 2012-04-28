<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
    <div>
        <%foreach (KarottoXros.Models.PlayerModels player in ViewData["PlayerList"] as List<KarottoXros.Models.PlayerModels>)
      {%>
        <div><%=Html.ActionLink(player.ChineseName, "Show", "Player", new {PlayerID = player.ID},null)%><%=Html.ActionLink("删除", "delete", "Player", new {PlayerID = player.ID},null)%></div>
    <%} %>
    </div>
</body>
</html>
