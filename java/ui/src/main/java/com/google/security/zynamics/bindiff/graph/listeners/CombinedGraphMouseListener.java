package com.google.security.zynamics.bindiff.graph.listeners;

import com.google.common.base.Preconditions;
import com.google.security.zynamics.bindiff.enums.EGraphType;
import com.google.security.zynamics.bindiff.graph.CombinedGraph;
import com.google.security.zynamics.bindiff.graph.edges.CombinedDiffEdge;
import com.google.security.zynamics.bindiff.graph.eventhandlers.GraphLayoutEventHandler;
import com.google.security.zynamics.bindiff.graph.nodes.CombinedDiffNode;
import com.google.security.zynamics.bindiff.gui.tabpanels.viewtabpanel.ViewTabPanelFunctions;
import com.google.security.zynamics.bindiff.gui.tabpanels.viewtabpanel.popupmenus.CallGraphPopupMenu;
import com.google.security.zynamics.bindiff.gui.tabpanels.viewtabpanel.popupmenus.FlowGraphPopupMenu;
import com.google.security.zynamics.bindiff.project.rawcallgraph.RawCombinedFunction;
import com.google.security.zynamics.bindiff.project.rawflowgraph.RawCombinedBasicBlock;
import com.google.security.zynamics.zylib.yfileswrap.gui.zygraph.IZyGraphListener;
import com.google.security.zynamics.zylib.yfileswrap.gui.zygraph.proximity.ZyProximityNode;
import java.awt.event.MouseEvent;
import javax.swing.JPopupMenu;
import javax.swing.SwingUtilities;
import y.view.EdgeLabel;

public class CombinedGraphMouseListener
    implements IZyGraphListener<CombinedDiffNode, CombinedDiffEdge> {
  private final CombinedGraph graph;

  private final ViewTabPanelFunctions controller;

  protected CombinedGraphMouseListener(
      final ViewTabPanelFunctions controller, final CombinedGraph graph) {
    this.controller = Preconditions.checkNotNull(controller);
    this.graph = Preconditions.checkNotNull(graph);

    addListener();
  }

  public void addListener() {
    graph.addListener(this);
  }

  @Override
  public void edgeClicked(
      final CombinedDiffEdge node, final MouseEvent event, final double x, final double y) {}

  @Override
  public void edgeLabelEntered(final EdgeLabel label, final MouseEvent event) {}

  @Override
  public void edgeLabelExited(final EdgeLabel label) {}

  @Override
  public void nodeClicked(
      final CombinedDiffNode node, final MouseEvent event, final double x, final double y) {
    if (SwingUtilities.isRightMouseButton(event)) {
      if (node.getRawNode() instanceof RawCombinedFunction) {
        final JPopupMenu menu = new CallGraphPopupMenu(controller, graph, node);
        menu.show(graph.getView(), event.getX(), event.getY());
      } else if (node.getRawNode() instanceof RawCombinedBasicBlock) {
        final JPopupMenu menu = new FlowGraphPopupMenu(controller, graph, node);
        menu.show(graph.getView(), event.getX(), event.getY());
      }
      return;
    }

    if (SwingUtilities.isLeftMouseButton(event)
        && event.getClickCount() == 2
        && graph.getGraphType() == EGraphType.CALLGRAPH) {
      controller.openFlowgraphsViews(node);
    }
  }

  @Override
  public void nodeEntered(final CombinedDiffNode node, final MouseEvent event) {}

  @Override
  public void nodeHovered(final CombinedDiffNode node, final double x, final double y) {}

  @Override
  public void nodeLeft(final CombinedDiffNode node) {}

  @Override
  public void proximityBrowserNodeClicked(
      final ZyProximityNode<?> proximityNode,
      final MouseEvent event,
      final double x,
      final double y) {
    if (SwingUtilities.isLeftMouseButton(event)) {
      GraphLayoutEventHandler.handleProximityNodeClickedEvent(graph, proximityNode);
    }
  }

  public void removeListener() {
    graph.removeListener(this);
  }
}