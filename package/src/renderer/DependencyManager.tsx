import type { RefObject } from "react";

import type { SkiaView } from "../views";
import type { SkiaValue } from "../values";

import type { Node } from "./nodes";

type Unsubscribe = () => void;
type SubscriptionInfo = {
  value: SkiaValue<unknown>;
  key: string | symbol | number;
  listener: (s: unknown) => void;
};
type Subscription = {
  value: SkiaValue<unknown>;
  listeners: Array<(v: unknown) => void>;
  unsubscribe: null | Unsubscribe;
};

export class DependencyManager {
  ref: RefObject<SkiaView>;
  nodeSubscriptionInfos: Map<Node, SubscriptionInfo[]> = new Map();
  valueSubscriptions: Map<SkiaValue<unknown>, Subscription> = new Map();
  unregisterRedrawRequests: null | Unsubscribe = null;

  constructor(ref: RefObject<SkiaView>) {
    this.ref = ref;
  }

  /**
   * Call to unsubscribe all value listeners from the given node based
   * on the current list of subscriptions for the node. This function
   * is typically called when the node is unmounted or when one or more
   * properties have changed.
   * @param node Node to unsubscribe value listeners from
   */
  unsubscribeNode(node: Node) {
    // console.log("DependencyMngr: unsubscribeNode", node.nodeId);
    const subscriptions = this.nodeSubscriptionInfos.get(node);
    if (subscriptions) {
      // console.log(
      //   "DependencyMngr: unsubscribeNode",
      //   node.nodeId,
      //   "-> subscriptions",
      //   subscriptions.map((p) => p.key).join(", ")
      // );
      subscriptions.forEach((si) => {
        // Remove from listeners in subscribed value
        this.valueSubscriptions.forEach((s) => {
          s.listeners = s.listeners.filter((l) => l !== si.listener);
        });

        // Remove subscription if there are no listeneres left on the value
        const valueSubscription = this.valueSubscriptions.get(si.value);
        if (valueSubscription && valueSubscription.listeners.length === 0) {
          // console.log(
          //   "DependencyMngr: unsubscribeNode",
          //   node.nodeId,
          //   " -> this.valueSubscriptions.delete",
          //   this.valueSubscriptions.size
          // );
          // Unsubscribe
          if (!valueSubscription.unsubscribe) {
            throw new Error("Failed to unsubscribe to value subscription");
          }
          valueSubscription.unsubscribe && valueSubscription.unsubscribe();

          // Remove from value subscriptions
          if (!this.valueSubscriptions.delete(si.value)) {
            throw new Error("Failed to delete value subscription info");
          }
        }
      });

      // Remove node's subscription info
      if (!this.nodeSubscriptionInfos.delete(node)) {
        throw new Error("Failed to delete node subscription info");
      }

      // Remove node
      node.removeNode();
    }
  }

  /**
   * Adds listeners to the provided values so that the node is notified
   * when a value changes. This is done in an optimized way so that this
   * class only needs to listen to the value once and then forwards the
   * change to the node and its listener. This method is typically called
   * when the node is mounted and when one or more props on the node changes.
   * @param node Node to subscribe to value changes for
   * @param subscriptionInfos Subscription information
   */
  subscribeNode(node: Node, subscriptionInfos: SubscriptionInfo[]) {
    if (subscriptionInfos.length === 0) {
      return;
    }
    // console.log(
    //   `DependencyMngr: subscribeNode ${node.nodeId} -> subscriptions:`,
    //   subscriptionInfos.map((p) => p.key).join(", "),
    //   "(" + subscriptionInfos.length + ")"
    // );
    // Install all subscriptions
    subscriptionInfos.forEach((si) => {
      // Do we already have this one as a unique value?
      let valueSubscription = this.valueSubscriptions.get(si.value);
      if (!valueSubscription) {
        // console.log(
        //   `DependencyMngr: subscribeNode ${node.nodeId} -> no subscription found for value`
        // );
        // Subscribe to the value
        valueSubscription = {
          value: si.value,
          listeners: [],
          unsubscribe: null,
        };
        // Add single subscription to the new value
        valueSubscription.unsubscribe = si.value.addListener((v) => {
          valueSubscription!.listeners.forEach((listener) => listener(v));
        });
        // console.log(
        //   `DependencyMngr: subscribeNode ${node.nodeId} -> this.valueSubscriptions.set`
        // );
        this.valueSubscriptions.set(si.value, valueSubscription);
      }
      // Add listener
      valueSubscription.listeners.push(si.listener);
      // console.log(
      //   `DependencyMngr: subscribeNode ${node.nodeId} -> number of value subscriptions:`,
      //   valueSubscription.listeners.length
      // );
    });
    // Save node's subscription info as well
    this.nodeSubscriptionInfos.set(node, subscriptionInfos);
  }

  /**
   * Called when the hosting container is mounted or updated. This ensures that we have
   * a ref to the underlying SkiaView so that we can registers redraw listeners
   * on values used in the current View automatically.
   */
  update() {
    if (this.ref.current === null) {
      throw new Error("Canvas ref is not set");
    }

    // console.log(
    //   "DependencyMngr: update -> Subscribed values:",
    //   this.valueSubscriptions.size,
    //   "/ Nodes:",
    //   Array.from(this.nodeSubscriptionInfos)
    //     .map((p) => p[0].nodeId)
    //     .join(", ")
    // );

    // Remove any previous registrations
    if (this.unregisterRedrawRequests) {
      this.unregisterRedrawRequests();
    }

    // Register redraw requests on the SkiaView for each unique value
    this.unregisterRedrawRequests = this.ref.current.registerValues(
      Array.from(this.valueSubscriptions.keys())
    );
  }

  /**
   * Called when the hosting container is unmounted or recreated. This ensures that we remove
   * all subscriptions to Skia values so that we don't have any listeners left after
   * the component is removed.
   */
  remove() {
    // console.log("DependencyMngr: remove");

    // 1) Unregister redraw requests
    if (this.unregisterRedrawRequests) {
      this.unregisterRedrawRequests();
      this.unregisterRedrawRequests = null;
    }

    // 2) Unregister nodes
    this.nodeSubscriptionInfos.forEach((_, node) => this.unsubscribeNode(node));
    this.nodeSubscriptionInfos.clear();
    this.valueSubscriptions.clear();
  }
}
