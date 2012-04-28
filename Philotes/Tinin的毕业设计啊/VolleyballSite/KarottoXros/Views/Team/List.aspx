<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
    <div>
    <%foreach (KarottoXros.Models.TeamModels team in ViewData["TeamList"] as List<KarottoXros.Models.TeamModels>)
      {%>
      <div><%=Html.ActionLink(team.City + team.Name, "Show", "Team", new {TId = team.Id},null)%></div>
    <%} %>
    </div>
</body>
</html>
