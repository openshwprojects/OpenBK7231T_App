using System.Collections.Generic;
using Lextm.SharpSnmpLib.Mib.Elements.Entities;

namespace Lextm.SharpSnmpLib.Mib
{
    /// <summary>
    /// Builds up a tree from a single MIB
    /// </summary>
    public class MibTree
    {
        private readonly List<MibTreeNode> _root = new List<MibTreeNode>();

        public MibTree(MibModule module)
        {
            IList<IEntity> entities = module.Entities;

            if (entities.Count > 0)
            {
                // try to find module identity as root
                foreach (IEntity element in entities)
                {
                    ModuleIdentity mi = element as ModuleIdentity;

                    if (mi != null)
                    {
                        _root.Add(new MibTreeNode(null, mi));
                    }
                }

                // gather all items below ModuleIdentity
                foreach (MibTreeNode mibTreeNode in _root)
                {
					entities.Remove (mibTreeNode.Entity);
                    BuildTree(mibTreeNode, entities);
                    UpdateTreeNodeTypes(mibTreeNode);
                }

                // find OID assignments as root, if there are any that are not below ModuleIdentity
                // FIXME: There may be multiple OID assignments that create a tree (find the root ones!)
                foreach (IEntity element in entities)
                {
                    OidValueAssignment oa = element as OidValueAssignment;

                    if (oa != null)
                    {
                        _root.Add(new MibTreeNode(null, oa));
                    }
                }

                if (_root.Count == 0)
                {
                    //no module identity, assume first entity is root
                    _root.Add(new MibTreeNode(null, entities[0]));
                }

                foreach (MibTreeNode mibTreeNode in _root)
                {
					if (entities.Contains (mibTreeNode.Entity))
					{
						entities.Remove (mibTreeNode.Entity);
					}
                    BuildTree(mibTreeNode, entities);
                    UpdateTreeNodeTypes(mibTreeNode);
                }
            }
        }

        public IList<MibTreeNode> Root
        {
            get { return _root; }
        }


        private void BuildTree(MibTreeNode node, IList<IEntity> entities)
        {
            int i = 0;
            while (i < entities.Count)
            {
                if (entities[i].Parent == node.Entity.Name)
                {
                    node.AddChild(entities[i]);
                    entities.RemoveAt(i);
                }
                else
                {
                    i++;
                }
            }

            foreach (MibTreeNode childNode in node.ChildNodes)
            {
                BuildTree(childNode, entities);
            }
        }
        
        private void UpdateTreeNodeTypes(MibTreeNode node)
        {
            node.UpdateNodeType();
            foreach (MibTreeNode childNode in node.ChildNodes)
            {
                UpdateTreeNodeTypes(childNode);
            }
        }
    }
}
